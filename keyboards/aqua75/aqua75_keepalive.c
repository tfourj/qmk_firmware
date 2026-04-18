// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_keepalive.h"

#include "aqua75_os.h"
#include "aqua75_shared.h"
#include "raw_hid.h"
#include "rgblight.h"
#include "sync_timer.h"
#include <string.h>

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
