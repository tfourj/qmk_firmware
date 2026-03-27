// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "timer.h"

#define AQUA75_RGB_IDLE_TIMEOUT 300000
#define AQUA75_HUE_GREEN 85
#define AQUA75_HUE_RED 0
#define AQUA75_HUE_YELLOW 43
#define AQUA75_HUE_CYAN 128
#define AQUA75_HUE_MAGENTA 191
#define AQUA75_HUE_THRESHOLD 24
#define AQUA75_CAPS_LED_INDEX 47

static bool     aqua75_capslock_active  = false;
static bool     aqua75_capslock_visible = false;
static bool     aqua75_is_suspended     = false;
static bool     aqua75_rgb_idle_off     = false;
static bool     aqua75_rgb_was_enabled  = false;
static uint32_t aqua75_capslock_timer   = 0;
static uint32_t aqua75_last_input_time  = 0;
static uint8_t  aqua75_capslock_hue     = AQUA75_HUE_GREEN;

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
    rgblight_set();
    if (enabled && rgblight_is_enabled()) {
        rgblight_sethsv_at(aqua75_capslock_hue, rgblight_get_sat(), rgblight_get_val(), AQUA75_CAPS_LED_INDEX);
    }
}

void keyboard_post_init_kb(void) {
    aqua75_update_capslock_layer(false);
    aqua75_last_input_time = last_input_activity_time();
    keyboard_post_init_user();
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

    if (current_input_time != aqua75_last_input_time) {
        aqua75_last_input_time = current_input_time;

        if (aqua75_rgb_idle_off) {
            aqua75_rgb_idle_off = false;
            if (aqua75_rgb_was_enabled) {
                rgblight_enable_noeeprom();
            }
        }
    }

    if (!aqua75_is_suspended && !aqua75_rgb_idle_off && rgblight_is_enabled() && last_input_activity_elapsed() >= AQUA75_RGB_IDLE_TIMEOUT) {
        aqua75_rgb_was_enabled = true;
        aqua75_rgb_idle_off    = true;
        aqua75_update_capslock_layer(false);
        rgblight_disable_noeeprom();
    }

    if (aqua75_capslock_active && !aqua75_is_suspended && !aqua75_rgb_idle_off && rgblight_is_enabled()) {
        if (timer_elapsed32(aqua75_capslock_timer) >= 500) {
            aqua75_capslock_timer = timer_read32();
            aqua75_update_capslock_layer(!aqua75_capslock_visible);
        }
    } else if (aqua75_capslock_visible) {
        aqua75_update_capslock_layer(false);
    }

    housekeeping_task_user();
}

void suspend_power_down_kb(void) {
    aqua75_is_suspended = true;
    aqua75_update_capslock_layer(false);
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    aqua75_is_suspended   = false;
    aqua75_capslock_timer = timer_read32();
    aqua75_last_input_time = last_input_activity_time();

    if (aqua75_capslock_active && !aqua75_rgb_idle_off && rgblight_is_enabled()) {
        aqua75_update_capslock_layer(true);
    }

    suspend_wakeup_init_user();
}
