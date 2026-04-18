// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_keepalive.h"

#include "aqua75_os.h"
#include "aqua75_rgb.h"
#include "aqua75_shared.h"
#include "raw_hid.h"
#include "rgblight.h"
#include "sync_timer.h"
#include <string.h>

bool aqua75_via_mode_enabled(void) {
    return aqua75_state.via_mode_enabled;
}

void aqua75_set_via_mode_enabled(bool enabled) {
    if (aqua75_state.via_mode_enabled == enabled) {
        return;
    }

    aqua75_state.via_mode_enabled = enabled;
    if (enabled) {
        aqua75_state.host_os_hint = OS_UNSURE;
    }
    aqua75_state.rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(aqua75_current_host_os());
    aqua75_rgb_handle_detected_os();
}

bool aqua75_process_record_mode(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case RGB_MD:
            aqua75_set_via_mode_enabled(false);
            return false;
        case VIA_MD:
            aqua75_set_via_mode_enabled(true);
            return false;
        default:
            return true;
    }
}

void aqua75_rgb_keepalive_ping(uint8_t host_os_hint) {
    aqua75_state.host_keepalive_time = sync_timer_read32();
    aqua75_set_keepalive_host_os(host_os_hint);

    if (aqua75_state.rgb_idle_off) {
        aqua75_state.rgb_idle_off = false;
        if (aqua75_state.rgb_was_enabled) {
            rgblight_enable_noeeprom();
        }
    }
}

bool aqua75_raw_hid_receive(uint8_t *data, uint8_t length) {
    uint8_t response[32];

    if (aqua75_via_mode_enabled()) {
        return false;
    }

    if (length != sizeof(response)) {
        return false;
    }

    memset(response, 0, sizeof(response));

    switch (data[0]) {
        case AQUA75_RAW_HID_CMD_KEEPALIVE:
            aqua75_rgb_keepalive_ping(data[2]);
            response[0] = AQUA75_RAW_HID_RSP_KEEPALIVE_ACK;
            response[1] = data[1];
            response[2] = rgblight_is_enabled() ? 1 : 0;
            response[3] = aqua75_current_host_os();
            raw_hid_send(response, sizeof(response));
            return true;
        default:
            return false;
    }
}
