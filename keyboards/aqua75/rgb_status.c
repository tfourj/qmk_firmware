// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_keepalive.h"
#include "aqua75_os.h"
#include "aqua75_rgb.h"
#include "eeconfig.h"

#if defined(VIA_ENABLE)
#    include "via.h"
#    include "raw_hid.h"
#else
#    include "raw_hid.h"
#endif

void keyboard_post_init_kb(void) {
    aqua75_load_mode_config();
    aqua75_rgb_post_init();
    keyboard_post_init_user();
}

void eeconfig_init_kb(void) {
    aqua75_eeconfig_init_mode();
    eeconfig_init_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    return aqua75_process_record_mode(keycode, record);
}

bool led_update_kb(led_t led_state) {
    if (!led_update_user(led_state)) {
        return false;
    }

    return aqua75_rgb_led_update(led_state);
}

void housekeeping_task_kb(void) {
    aqua75_rgb_housekeeping();
    housekeeping_task_user();
}

#if defined(VIA_ENABLE)
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
            if (value_id_and_data[0] != id_aqua75_usb_reset) {
                *command_id = id_unhandled;
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
#endif

void suspend_power_down_kb(void) {
    aqua75_rgb_suspend_power_down();
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    aqua75_rgb_suspend_wakeup_init();
    suspend_wakeup_init_user();
}

#if defined(VIA_ENABLE)
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (!aqua75_via_mode_enabled()) {
        if (aqua75_raw_hid_receive(data, length)) {
            return true;
        }

        data[0] = id_unhandled;
        raw_hid_send(data, length);
        return true;
    }

    return false;
}
#else
void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (aqua75_raw_hid_receive(data, length)) {
        return;
    }
}
#endif
