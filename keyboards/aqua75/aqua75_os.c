// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_os.h"
#include "aqua75_rgb.h"

static os_variant_t aqua75_keepalive_host_os_to_variant(uint8_t host_os_hint) {
    switch (host_os_hint) {
        case AQUA75_KEEPALIVE_OS_LINUX:
            return OS_LINUX;
        case AQUA75_KEEPALIVE_OS_WINDOWS:
            return OS_WINDOWS;
        case AQUA75_KEEPALIVE_OS_MACOS:
            return OS_MACOS;
        case AQUA75_KEEPALIVE_OS_UNSURE:
        default:
            return OS_UNSURE;
    }
}

os_variant_t aqua75_current_host_os(void) {
    if (aqua75_state.host_os_hint != OS_UNSURE) {
        return aqua75_state.host_os_hint;
    }

    return detected_host_os();
}

uint32_t aqua75_rgb_idle_timeout_for_os(os_variant_t detected_os) {
    (void)detected_os;
    return AQUA75_RGB_IDLE_TIMEOUT_MS;
}

void aqua75_set_keepalive_host_os(uint8_t host_os_hint) {
    aqua75_state.host_os_hint     = aqua75_keepalive_host_os_to_variant(host_os_hint);
    aqua75_state.rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(aqua75_current_host_os());
    aqua75_rgb_handle_detected_os();
}

bool aqua75_process_detected_host_os(os_variant_t detected_os) {
    aqua75_state.rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(aqua75_current_host_os());
    aqua75_rgb_handle_detected_os();
    return process_detected_host_os_user(detected_os);
}
