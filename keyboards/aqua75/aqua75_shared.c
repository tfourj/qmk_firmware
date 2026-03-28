// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_shared.h"
#include "led_map.h"

aqua75_state_t aqua75_state = {
    .rgb_idle_timeout = AQUA75_RGB_IDLE_TIMEOUT_SHORT,
    .capslock_hue     = AQUA75_HUE_GREEN,
    .fn_indicator_led = AQUA75_NO_LED,
};
