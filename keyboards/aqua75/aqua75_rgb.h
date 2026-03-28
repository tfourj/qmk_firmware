// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "aqua75_shared.h"

void aqua75_rgb_post_init(void);
void aqua75_rgb_handle_detected_os(void);
bool aqua75_rgb_led_update(led_t led_state);
void aqua75_rgb_housekeeping(void);
void aqua75_rgb_suspend_power_down(void);
void aqua75_rgb_suspend_wakeup_init(void);
