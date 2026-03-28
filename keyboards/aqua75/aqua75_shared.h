// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"
#include "os_detection.h"

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
#define AQUA75_MANUAL_USB_DISCONNECT_MS 250
#define AQUA75_KVM_TAP_HOLD_DELAY 80
#define AQUA75_KVM_DOUBLE_TAP_DELAY 150
#define AQUA75_KVM_SELECT_DELAY 185
#define AQUA75_KVM_SEQUENCE_SETTLE_DELAY 80
#define AQUA75_KVM_RESET_ARM_DELAY 300
#define AQUA75_KVM_RESET_TIMEOUT 2000

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

typedef struct {
    bool     capslock_active;
    bool     capslock_visible;
    bool     fn_indicator_visible;
    bool     fn_was_held;
    bool     is_suspended;
    bool     rgb_idle_off;
    bool     rgb_was_enabled;
    bool     ignore_fn_activity;
    bool     manual_reset_pending;
    bool     kvm_reset_pending;
    bool     kvm_reset_armed;
    uint32_t capslock_timer;
    uint32_t fn_indicator_timer;
    uint32_t fn_tap_timer;
    uint32_t last_input_time;
    uint32_t manual_reset_timer;
    uint32_t kvm_reset_timer;
    uint32_t rgb_idle_timeout;
    uint8_t  capslock_hue;
    uint8_t  fn_indicator_led;
} aqua75_state_t;

extern aqua75_state_t aqua75_state;
