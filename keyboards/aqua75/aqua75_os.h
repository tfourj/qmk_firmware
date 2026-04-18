// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "aqua75_shared.h"

uint32_t aqua75_rgb_idle_timeout_for_os(os_variant_t detected_os);
void     aqua75_set_keepalive_host_os(uint8_t host_os_hint);
os_variant_t aqua75_current_host_os(void);
void     aqua75_expire_keepalive_host_os(void);
