// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "aqua75_kvm.h"
#include "timer.h"
#include "usb_util.h"

static void aqua75_schedule_manual_reset(void) {
    aqua75_state.manual_reset_pending = true;
    aqua75_state.manual_reset_timer   = timer_read32();
}

static void aqua75_schedule_kvm_reset(void) {
    aqua75_state.kvm_reset_pending = true;
    aqua75_state.kvm_reset_armed   = false;
    aqua75_state.kvm_reset_timer   = timer_read32();
}

static void aqua75_kvm_tap(uint16_t keycode, uint16_t hold_delay, uint16_t release_delay) {
    register_code16(keycode);
    wait_ms(hold_delay);
    unregister_code16(keycode);
    wait_ms(release_delay);
}

static void aqua75_kvm_input(uint16_t keycode) {
    clear_keyboard();
    wait_ms(AQUA75_KVM_SEQUENCE_SETTLE_DELAY);
    aqua75_kvm_tap(KC_RCTL, AQUA75_KVM_TAP_HOLD_DELAY, AQUA75_KVM_DOUBLE_TAP_DELAY);
    aqua75_kvm_tap(KC_RCTL, AQUA75_KVM_TAP_HOLD_DELAY, AQUA75_KVM_SELECT_DELAY);
    aqua75_kvm_tap(keycode, AQUA75_KVM_TAP_HOLD_DELAY, AQUA75_KVM_SEQUENCE_SETTLE_DELAY);
    clear_keyboard();
    aqua75_schedule_kvm_reset();
}

bool aqua75_process_record_kvm(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case USB_RST:
            aqua75_schedule_manual_reset();
            return false;
        case KVM_IN1:
            aqua75_kvm_input(KC_1);
            return false;
        case KVM_IN2:
            aqua75_kvm_input(KC_2);
            return false;
        default:
            return true;
    }
}

bool aqua75_kvm_housekeeping(void) {
    if (aqua75_state.manual_reset_pending && timer_elapsed32(aqua75_state.manual_reset_timer) >= AQUA75_MANUAL_RESET_DELAY) {
        aqua75_state.manual_reset_pending = false;
        usb_disconnect();
        wait_ms(AQUA75_MANUAL_USB_DISCONNECT_MS);
        soft_reset_keyboard();
        return false;
    }

    if (aqua75_state.kvm_reset_pending) {
        uint32_t elapsed = timer_elapsed32(aqua75_state.kvm_reset_timer);

        if (elapsed >= AQUA75_KVM_RESET_TIMEOUT) {
            aqua75_state.kvm_reset_pending = false;
            aqua75_state.kvm_reset_armed   = false;
        } else if (elapsed >= AQUA75_KVM_RESET_ARM_DELAY) {
            aqua75_state.kvm_reset_armed = true;
        }
    }

    return true;
}

void aqua75_kvm_handle_detected_os(os_variant_t detected_os) {
    if (aqua75_state.kvm_reset_pending && aqua75_state.kvm_reset_armed && detected_os != OS_UNSURE) {
        aqua75_state.kvm_reset_pending = false;
        aqua75_state.kvm_reset_armed   = false;
        aqua75_schedule_manual_reset();
    }
}

void aqua75_via_custom_value_command(uint8_t *data, uint8_t length) {
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id != id_aqua75_channel) {
        *command_id = id_unhandled;
        return;
    }

    switch (*command_id) {
        case id_custom_set_value:
            if (value_id_and_data[0] == id_aqua75_usb_reset && value_id_and_data[1] == 1) {
                aqua75_schedule_manual_reset();
            }
            break;
        case id_custom_get_value:
            if (value_id_and_data[0] == id_aqua75_usb_reset) {
                value_id_and_data[1] = 0;
            } else {
                *command_id = id_unhandled;
            }
            break;
        case id_custom_save:
            break;
        default:
            *command_id = id_unhandled;
            break;
    }

    (void)length;
}
