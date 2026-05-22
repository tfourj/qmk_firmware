// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_keepalive.h"
#include "aqua75_rgb.h"
#include "aqua75_os.h"
#include "led_map.h"
#include "print.h"
#include "sync_timer.h"
#include "timer.h"

static uint32_t aqua75_last_activity_time(void) {
    uint32_t keyboard_activity_time = last_input_activity_time();
    uint32_t host_keepalive_time    = aqua75_state.host_keepalive_time;

    if ((int32_t)(host_keepalive_time - keyboard_activity_time) > 0) {
        return host_keepalive_time;
    }

    return keyboard_activity_time;
}

static bool aqua75_underglow_is_white(void) {
    return rgblight_get_sat() == 0 && rgblight_get_val() > 0;
}

static void aqua75_set_status_blink_at(uint8_t led_index) {
    if (led_index == AQUA75_NO_LED) {
        return;
    }

    if (aqua75_underglow_is_white()) {
        rgblight_sethsv_at(AQUA75_HUE_RED, 255, rgblight_get_val(), led_index);
    } else {
        rgblight_sethsv_at(0, 0, rgblight_get_val(), led_index);
    }
}

static void aqua75_update_capslock_layer(bool enabled) {
    aqua75_state.capslock_visible = enabled;
    if (!rgblight_is_enabled()) {
        return;
    }

    if (aqua75_state.capslock_active && enabled) {
        aqua75_set_status_blink_at(AQUA75_CAPS_LED_INDEX);
    } else {
        rgblight_sethsv_at(rgblight_get_hue(), rgblight_get_sat(), rgblight_get_val(), AQUA75_CAPS_LED_INDEX);
    }
}

static void aqua75_restore_led_color(uint8_t led_index) {
    if (led_index == AQUA75_NO_LED) {
        return;
    }

    rgblight_sethsv_at(rgblight_get_hue(), rgblight_get_sat(), rgblight_get_val(), led_index);
}

static uint8_t aqua75_fn_indicator_led_index(void) {
    if (aqua75_via_mode_enabled()) {
        return aqua75_matrix_to_led(AQUA75_V_ROW, AQUA75_V_COL);
    }

    return aqua75_matrix_to_led(AQUA75_C_ROW, AQUA75_C_COL);
}

static uint8_t aqua75_fn_os_indicator_led_index(void) {
    switch (aqua75_current_host_os()) {
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
    uint8_t mode_led_index = aqua75_fn_indicator_led_index();
    uint8_t os_led_index   = aqua75_fn_os_indicator_led_index();

    aqua75_state.fn_indicator_visible = enabled;
    if (!rgblight_is_enabled()) {
        aqua75_state.fn_mode_indicator_led = mode_led_index;
        aqua75_state.fn_os_indicator_led   = os_led_index;
        return;
    }

    if (aqua75_state.fn_mode_indicator_led != mode_led_index) {
        aqua75_restore_led_color(aqua75_state.fn_mode_indicator_led);
        aqua75_state.fn_mode_indicator_led = mode_led_index;
    }

    if (aqua75_state.fn_os_indicator_led != os_led_index) {
        aqua75_restore_led_color(aqua75_state.fn_os_indicator_led);
        aqua75_state.fn_os_indicator_led = os_led_index;
    }

    if (enabled) {
        if (mode_led_index != AQUA75_NO_LED) {
            aqua75_set_status_blink_at(mode_led_index);
        }
        if (os_led_index != AQUA75_NO_LED && os_led_index != mode_led_index) {
            aqua75_set_status_blink_at(os_led_index);
        }
    } else {
        if (mode_led_index != AQUA75_NO_LED) {
            aqua75_restore_led_color(mode_led_index);
        }
        if (os_led_index != AQUA75_NO_LED && os_led_index != mode_led_index) {
            aqua75_restore_led_color(os_led_index);
        }
    }
}

static void aqua75_force_rgb_idle_off(void) {
    if (aqua75_state.rgb_idle_off || !rgblight_is_enabled()) {
        return;
    }

    aqua75_state.rgb_was_enabled    = true;
    aqua75_state.rgb_idle_off       = true;
    aqua75_state.ignore_fn_activity = true;
    aqua75_state.last_input_time    = aqua75_last_activity_time();
    aqua75_update_capslock_layer(false);
    aqua75_update_fn_indicator(false);
    rgblight_disable_noeeprom();
}

void aqua75_rgb_post_init(void) {
    aqua75_update_capslock_layer(false);
    aqua75_update_fn_indicator(false);
    aqua75_state.last_input_time  = aqua75_last_activity_time();
    aqua75_state.rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(aqua75_current_host_os());
}

void aqua75_rgb_handle_detected_os(void) {
    if (aqua75_state.fn_indicator_visible) {
        aqua75_update_fn_indicator(true);
    } else {
        aqua75_state.fn_mode_indicator_led = aqua75_fn_indicator_led_index();
        aqua75_state.fn_os_indicator_led   = aqua75_fn_os_indicator_led_index();
    }
}

bool aqua75_rgb_led_update(led_t led_state) {
    aqua75_state.capslock_active = led_state.caps_lock;
    aqua75_state.capslock_timer  = timer_read32();

    if (aqua75_state.capslock_active && !aqua75_state.is_suspended && rgblight_is_enabled()) {
        aqua75_update_capslock_layer(true);
    } else {
        aqua75_update_capslock_layer(false);
    }

    return true;
}

void aqua75_rgb_housekeeping(void) {
    uint32_t current_input_time = aqua75_last_activity_time();
    bool     fn_held            = matrix_is_on(AQUA75_FN_ROW, AQUA75_FN_COL);

    aqua75_expire_keepalive_host_os();

    if (current_input_time != aqua75_state.last_input_time) {
        aqua75_state.last_input_time = current_input_time;

        if (aqua75_state.ignore_fn_activity && !fn_held) {
            aqua75_state.ignore_fn_activity = false;
        } else if (aqua75_state.rgb_idle_off) {
            aqua75_state.rgb_idle_off = false;
            if (aqua75_state.rgb_was_enabled) {
                rgblight_enable_noeeprom();
            }
        }
    }

    if (!aqua75_state.is_suspended && !aqua75_state.rgb_idle_off && rgblight_is_enabled() && sync_timer_elapsed32(current_input_time) >= aqua75_state.rgb_idle_timeout) {
        aqua75_force_rgb_idle_off();
    }

    if (aqua75_state.capslock_active && !aqua75_state.is_suspended && !aqua75_state.rgb_idle_off && rgblight_is_enabled()) {
        if (timer_elapsed32(aqua75_state.capslock_timer) >= AQUA75_STATUS_BLINK_INTERVAL) {
            aqua75_state.capslock_timer = timer_read32();
            aqua75_update_capslock_layer(!aqua75_state.capslock_visible);
        }
    } else if (aqua75_state.capslock_visible) {
        aqua75_update_capslock_layer(false);
    }

    if (fn_held && !aqua75_state.is_suspended && !aqua75_state.rgb_idle_off && rgblight_is_enabled()) {
        if (!aqua75_state.fn_was_held) {
            bool forced_idle_off = false;

            if (timer_elapsed32(aqua75_state.fn_tap_timer) <= AQUA75_FN_DOUBLE_TAP_TERM) {
                aqua75_state.fn_tap_timer = 0;
#ifdef CONSOLE_ENABLE
                uprintf("aqua75: fn double tap detected\n");
#endif
                aqua75_force_rgb_idle_off();
                forced_idle_off = true;
            } else {
                aqua75_state.fn_tap_timer = timer_read32();
            }

            aqua75_state.fn_was_held = true;
            if (!forced_idle_off) {
                aqua75_state.fn_indicator_timer = timer_read32();
                aqua75_update_fn_indicator(true);
            }
        } else if (timer_elapsed32(aqua75_state.fn_indicator_timer) >= AQUA75_STATUS_BLINK_INTERVAL) {
            aqua75_state.fn_indicator_timer = timer_read32();
            aqua75_update_fn_indicator(!aqua75_state.fn_indicator_visible);
        }
    } else {
        aqua75_state.fn_was_held = false;
        if (aqua75_state.fn_indicator_visible) {
            aqua75_update_fn_indicator(false);
        }
    }

    if (aqua75_state.i2c_recovery_flash > 0) {
        if (!rgblight_is_enabled()) {
            rgblight_enable_noeeprom();
        }
        rgblight_setrgb(0, 255, 0);
        aqua75_state.i2c_recovery_flash--;
    }
}

void aqua75_rgb_suspend_power_down(void) {
    aqua75_state.is_suspended = true;
    aqua75_state.fn_was_held  = false;
    aqua75_update_capslock_layer(false);
    aqua75_update_fn_indicator(false);
}

void aqua75_rgb_suspend_wakeup_init(void) {
    aqua75_state.is_suspended       = false;
    aqua75_state.capslock_timer     = timer_read32();
    aqua75_state.fn_indicator_timer = timer_read32();
    aqua75_state.last_input_time    = aqua75_last_activity_time();

    if (aqua75_state.capslock_active && !aqua75_state.rgb_idle_off && rgblight_is_enabled()) {
        aqua75_update_capslock_layer(true);
    }
}
