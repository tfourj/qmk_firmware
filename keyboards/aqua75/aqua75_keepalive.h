// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

#include <stdbool.h>
#include <stdint.h>

void aqua75_rgb_keepalive_ping(uint8_t host_os_hint);
bool aqua75_raw_hid_receive(uint8_t *data, uint8_t length);
bool aqua75_process_record_mode(uint16_t keycode, keyrecord_t *record);
bool aqua75_via_mode_enabled(void);
void aqua75_set_via_mode_enabled(bool enabled);
