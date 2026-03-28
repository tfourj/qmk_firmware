// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "aqua75_shared.h"

uint32_t aqua75_rgb_idle_timeout_for_os(os_variant_t detected_os);
bool     aqua75_process_detected_host_os(os_variant_t detected_os);
