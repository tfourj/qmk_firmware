// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_os.h"
#include "aqua75_kvm.h"
#include "aqua75_rgb.h"

uint32_t aqua75_rgb_idle_timeout_for_os(os_variant_t detected_os) {
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

bool aqua75_process_detected_host_os(os_variant_t detected_os) {
    aqua75_state.rgb_idle_timeout = aqua75_rgb_idle_timeout_for_os(detected_os);
    aqua75_rgb_handle_detected_os();
    aqua75_kvm_handle_detected_os(detected_os);
    return process_detected_host_os_user(detected_os);
}
