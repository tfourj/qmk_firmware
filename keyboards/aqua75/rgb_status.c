// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "led_map.h"
#include "os_detection.h"
#include "timer.h"

#define AQUA75_RGB_IDLE_TIMEOUT_LONG  300000
#define AQUA75_RGB_IDLE_TIMEOUT_SHORT 20000
#define AQUA75_HUE_GREEN 85
#define AQUA75_HUE_RED 0
#define AQUA75_HUE_YELLOW 43
#define AQUA75_HUE_CYAN 128
#define AQUA75_HUE_MAGENTA 191
#define AQUA75_HUE_THRESHOLD 24
#define AQUA75_CAPS_LED_INDEX 47
#define AQUA75_FN_ROW 5
#define AQUA75_FN_COL 10
#define AQUA75_STATUS_BLINK_INTERVAL 500
#define AQUA75_FN_DOUBLE_TAP_TERM 300
#define AQUA75_MANUAL_RESET_DELAY 50

enum aqua75_via_channel {
    id_aqua75_channel = 10,
};

enum aqua75_via_value {
    id_aqua75_usb_reset = 1,
};

enum aqua75_custom_keycodes {
    USB_RST = QK_USER_0,
    KVM_IN1,
    KVM_IN2,
};

static bool     aqua75_capslock_active  = false;
static bool     aqua75_capslock_visible = false;
static bool     aqua75_fn_indicator_visible = false;
static bool     aqua75_fn_was_held      = false;
static bool     aqua75_is_suspended     = false;
static bool     aqua75_rgb_idle_off     = false;
static bool     aqua75_rgb_was_enabled  = false;
static bool     aqua75_ignore_fn_activity = false;
static bool     aqua75_manual_reset_pending = false;
static uint32_t aqua75_capslock_timer   = 0;
static uint32_t aqua75_fn_indicator_timer = 0;
static uint32_t aqua75_fn_tap_timer     = 0;
static uint32_t aqua75_last_input_time  = 0;
static uint32_t aqua75_manual_reset_timer = 0;
static uint32_t aqua75_rgb_idle_timeout = AQUA75_RGB_IDLE_TIMEOUT_SHORT;
static uint8_t  aqua75_capslock_hue     = AQUA75_HUE_GREEN;
static uint8_t  aqua75_fn_indicator_led = AQUA75_NO_LED;

static void aqua75_update_fn_indicator(bool enabled);

static void aqua75_schedule_manual_reset(void) {
    aqua75_manual_reset_pending = true;
    aqua75_manual_reset_timer   = timer_read32();
}

static void aqua75_kvm_input(uint16_t keycode) {
    tap_code(KC_RCTL);
    wait_ms(150);
    tap_code(KC_RCTL);
    wait_ms(150);
    tap_code16(keycode);
}

static uint32_t aqua75_rgb_idle_timeout_for_os(os_variant_t detected_os) {
    switch (detected_os) {
        case OS_WINDOWS:
        case OS_MACOS:
        case OS_LINUX:
        case OS_IOS:
            return AQUA75_RGB_IDLE_TIMEOUT_LONG;
        case OS_UNSURE:
        default:
            return AQUA75_RGB_IDLE_TIMEOUT_SHORT;
    }
}

static uint8_t aqua75_hue_distance(uint8_t a, uint8_t b) {
    uint8_t distance = a > b ? a - b : b - a;
    return distance < (uint8_t)(256 - distance) ? distance : (uint8_t)(256 - distance);
}

static void aqua75_refresh_capslock_color(void) {
    static const uint8_t palette[] = {AQUA75_HUE_GREEN, AQUA75_HUE_CYAN, AQUA75_HUE_MAGENTA, AQUA75_HUE_YELLOW, AQUA75_HUE_RED};
    uint8_t              current_hue = rgblight_get_hue();
    uint8_t              flash_hue   = AQUA75_HUE_GREEN;

    if (aqua75_hue_distance(current_hue, flash_hue) < AQUA75_HUE_THRESHOLD) {
        uint8_t start = (uint8_t)(timer_read32() % ARRAY_SIZE(palette));

        for (uint8_t i = 0; i < ARRAY_SIZE(palette); i++) {
            uint8_t candidate = palette[(start + i) % ARRAY_SIZE(palette)];
            if (aqua75_hue_distance(current_hue, candidate) >= AQUA75_HUE_THRESHOLD) {
                flash_hue = candidate;
                break;
            }
        }
    }

    aqua75_capslock_hue = flash_hue;
}

static void aqua75_update_capslock_layer(bool enabled) {
    aqua75_capslock_visible = enabled;
    if (!rgblight_is_enabled()) {
        return;
    }

    if (aqua75_capslock_active && enabled) {
        rgblight_sethsv_at(aqua75_capslock_hue, rgblight_get_sat(), rgblight_get_val(), AQUA75_CAPS_LED_INDEX);
    } else {
        rgblight_sethsv_at(rgblight_get_hue(), rgblight_get_sat(), rgblight_get_val(), AQUA75_CAPS_LED_INDEX);
    }
}

static void aqua75_restore_led_color(uint8_t led_index) {
    rgblight_sethsv_at(rgblight_get_hue(), rgblight_get_sat(), rgblight_get_val(), led_index);
}

static void aqua75_force_rgb_idle_off(void) {
    if (aqua75_rgb_idle_off || !rgblight_is_enabled()) {
        return;
    }

    aqua75_rgb_was_enabled   = true;
    aqua75_rgb_idle_off      = true;
    aqua75_ignore_fn_activity = true;
    aqua75_last_input_time   = last_input_activity_time();
    aqua75_update_capslock_layer(false);
    aqua75_update_fn_indicator(false);
    rgblight_disable_noeeprom();
}

static uint8_t aqua75_fn_indicator_led_index(void) {
    switch (detected_host_os()) {
        case OS_WINDOWS:
            return aqua75_matrix_to_led(5, 0);
        case OS_MACOS:
            return aqua75_matrix_to_led(4, 0);
        case OS_UNSURE:
        case OS_LINUX:
        case OS_IOS:
        default:
            return aqua75_matrix_to_led(1, 0);
    }
}

static void aqua75_update_fn_indicator(bool enabled) {
    uint8_t led_index = aqua75_fn_indicator_led_index();

    aqua75_fn_indicator_visible = enabled;
    if (!rgblight_is_enabled()) {
        aqua75_fn_indicator_led = led_index;
        return;
    }

    if (aqua75_fn_indicator_led != led_index) {
        aqua75_restore_led_color(aqua75_fn_indicator_led);
        aqua75_fn_indicator_led = led_index;
    }

    if (led_index == AQUA75_NO_LED) {
        return;
    }

    if (enabled) {
        rgblight_sethsv_at(0, 0, rgblight_get_val(), led_index);
    } else {
        aqua75_restore_led_color(led_index);
    }
}

void keyboard_post_init_kb(void) {
    aqua75_update_capslock_layer(false);
    aqua75_update_fn_indicator(false);
    aqua75_last_input_time  = last_input_activity_time();
    aqua75_rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(detected_host_os());
    keyboard_post_init_user();
}

bool process_detected_host_os_kb(os_variant_t detected_os) {
    aqua75_rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(detected_os);
    if (rgblight_is_enabled()) {
        aqua75_update_fn_indicator(false);
    }
    return process_detected_host_os_user(detected_os);
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case USB_RST:
            aqua75_schedule_manual_reset();
            return false;
        case KVM_IN1:
            aqua75_kvm_input(KC_1);
            return false;
        case KVM_IN2:
            aqua75_kvm_input(KC_2);
            return false;
        default:
            return true;
    }
}

bool led_update_kb(led_t led_state) {
    if (!led_update_user(led_state)) {
        return false;
    }

    aqua75_capslock_active = led_state.caps_lock;
    aqua75_capslock_timer  = timer_read32();

    if (aqua75_capslock_active && !aqua75_is_suspended && rgblight_is_enabled()) {
        aqua75_refresh_capslock_color();
        aqua75_update_capslock_layer(true);
    } else {
        aqua75_update_capslock_layer(false);
    }

    return true;
}

void housekeeping_task_kb(void) {
    uint32_t current_input_time = last_input_activity_time();
    bool     fn_held            = matrix_is_on(AQUA75_FN_ROW, AQUA75_FN_COL);

    if (aqua75_manual_reset_pending && timer_elapsed32(aqua75_manual_reset_timer) >= AQUA75_MANUAL_RESET_DELAY) {
        aqua75_manual_reset_pending = false;
        soft_reset_keyboard();
        return;
    }

    if (current_input_time != aqua75_last_input_time) {
        aqua75_last_input_time = current_input_time;

        if (aqua75_ignore_fn_activity && !fn_held) {
            aqua75_ignore_fn_activity = false;
        } else if (aqua75_rgb_idle_off) {
            aqua75_rgb_idle_off = false;
            if (aqua75_rgb_was_enabled) {
                rgblight_enable_noeeprom();
            }
        }
    }

    if (!aqua75_is_suspended && !aqua75_rgb_idle_off && rgblight_is_enabled() && last_input_activity_elapsed() >= aqua75_rgb_idle_timeout) {
        aqua75_force_rgb_idle_off();
    }

    if (aqua75_capslock_active && !aqua75_is_suspended && !aqua75_rgb_idle_off && rgblight_is_enabled()) {
        if (timer_elapsed32(aqua75_capslock_timer) >= AQUA75_STATUS_BLINK_INTERVAL) {
            aqua75_capslock_timer = timer_read32();
            aqua75_update_capslock_layer(!aqua75_capslock_visible);
        }
    } else if (aqua75_capslock_visible) {
        aqua75_update_capslock_layer(false);
    }

    if (fn_held && !aqua75_is_suspended && !aqua75_rgb_idle_off && rgblight_is_enabled()) {
        if (!aqua75_fn_was_held) {
            bool forced_idle_off = false;

            if (timer_elapsed32(aqua75_fn_tap_timer) <= AQUA75_FN_DOUBLE_TAP_TERM) {
                aqua75_fn_tap_timer = 0;
                aqua75_force_rgb_idle_off();
                forced_idle_off = true;
            } else {
                aqua75_fn_tap_timer = timer_read32();
            }
            aqua75_fn_was_held       = true;
            if (!forced_idle_off) {
                aqua75_fn_indicator_timer = timer_read32();
                aqua75_update_fn_indicator(true);
            }
        } else if (timer_elapsed32(aqua75_fn_indicator_timer) >= AQUA75_STATUS_BLINK_INTERVAL) {
            aqua75_fn_indicator_timer = timer_read32();
            aqua75_update_fn_indicator(!aqua75_fn_indicator_visible);
        }
    } else {
        aqua75_fn_was_held = false;
        if (aqua75_fn_indicator_visible) {
            aqua75_update_fn_indicator(false);
        }
    }

    housekeeping_task_user();
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id != id_aqua75_channel) {
        *command_id = id_unhandled;
        return;
    }

    switch (*command_id) {
        case id_custom_set_value:
            if (value_id_and_data[0] == id_aqua75_usb_reset && value_id_and_data[1] == 1) {
                aqua75_schedule_manual_reset();
            }
            break;
        case id_custom_get_value:
            if (value_id_and_data[0] == id_aqua75_usb_reset) {
                value_id_and_data[1] = 0;
            } else {
                *command_id = id_unhandled;
            }
            break;
        case id_custom_save:
            break;
        default:
            *command_id = id_unhandled;
            break;
    }

    (void)length;
}

void suspend_power_down_kb(void) {
    aqua75_is_suspended = true;
    aqua75_fn_was_held = false;
    aqua75_update_capslock_layer(false);
    aqua75_update_fn_indicator(false);
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    aqua75_is_suspended   = false;
    aqua75_capslock_timer   = timer_read32();
    aqua75_fn_indicator_timer = timer_read32();
    aqua75_last_input_time = last_input_activity_time();

    if (aqua75_capslock_active && !aqua75_rgb_idle_off && rgblight_is_enabled()) {
        aqua75_update_capslock_layer(true);
    }

    suspend_wakeup_init_user();
}
