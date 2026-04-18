// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_keepalive.h"
#include "aqua75_os.h"
#include "aqua75_rgb.h"

#if defined(VIA_ENABLE)
#    include "via.h"
#else
#    include "raw_hid.h"
#endif

void keyboard_post_init_kb(void) {
    aqua75_rgb_post_init();
    keyboard_post_init_user();
}

bool process_detected_host_os_kb(os_variant_t detected_os) {
    return aqua75_process_detected_host_os(detected_os);
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    return true;
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
    return aqua75_raw_hid_receive(data, length);
}
#else
void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (aqua75_raw_hid_receive(data, length)) {
        return;
    }
}
#endif
