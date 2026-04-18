// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"
#include "os_detection.h"

#define AQUA75_RGB_IDLE_TIMEOUT_MS    30000
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

enum aqua75_via_channel {
    id_aqua75_channel = 10,
};

enum aqua75_via_value {
    id_aqua75_usb_reset = 1,
};

enum aqua75_raw_hid_command {
    AQUA75_RAW_HID_CMD_KEEPALIVE = 0x7F,
};

enum aqua75_keepalive_host_os {
    AQUA75_KEEPALIVE_OS_UNSURE  = 0,
    AQUA75_KEEPALIVE_OS_LINUX   = 1,
    AQUA75_KEEPALIVE_OS_WINDOWS = 2,
    AQUA75_KEEPALIVE_OS_MACOS   = 3,
};

enum aqua75_raw_hid_response {
    AQUA75_RAW_HID_RSP_KEEPALIVE_ACK = 0x81,
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
    uint32_t capslock_timer;
    uint32_t fn_indicator_timer;
    uint32_t fn_tap_timer;
    uint32_t last_input_time;
    uint32_t host_keepalive_time;
    uint32_t rgb_idle_timeout;
    uint8_t  capslock_hue;
    uint8_t  fn_indicator_led;
    os_variant_t host_os_hint;
} aqua75_state_t;

extern aqua75_state_t aqua75_state;
