// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "timer.h"

#define AQUA75_RGB_IDLE_TIMEOUT 600000

static const rgblight_segment_t PROGMEM aqua75_capslock_layer[] = RGBLIGHT_LAYER_SEGMENTS(
    {47, 1, HSV_RED}
);

static const rgblight_segment_t *const PROGMEM aqua75_rgblight_layers[] = RGBLIGHT_LAYERS_LIST(
    aqua75_capslock_layer
);

static bool     aqua75_capslock_active  = false;
static bool     aqua75_capslock_visible = false;
static bool     aqua75_is_suspended     = false;
static bool     aqua75_rgb_idle_off     = false;
static bool     aqua75_rgb_was_enabled  = false;
static uint32_t aqua75_capslock_timer   = 0;
static uint32_t aqua75_last_input_time  = 0;

static void aqua75_update_capslock_layer(bool enabled) {
    aqua75_capslock_visible = enabled;
    rgblight_set_layer_state(0, enabled);
}

void keyboard_post_init_kb(void) {
    rgblight_layers = aqua75_rgblight_layers;
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
