// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>

void aqua75_rgb_keepalive_ping(uint8_t host_os_hint);
bool aqua75_raw_hid_receive(uint8_t *data, uint8_t length);
