// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "aqua75_shared.h"

bool aqua75_process_record_kvm(uint16_t keycode, keyrecord_t *record);
bool aqua75_kvm_housekeeping(void);
void aqua75_kvm_handle_detected_os(os_variant_t detected_os);
void aqua75_via_custom_value_command(uint8_t *data, uint8_t length);
