// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include <avr/io.h>

#include "quantum.h"
#include "i2c_master.h"

#ifndef F_SCL
#    define F_SCL 400000UL
#endif

#define TWBR_VAL (((F_CPU / F_SCL) - 16) / 2)

void i2c_init(void) {
    // The original nice!nano setup enabled pull-ups on the I2C lines.
    // Keep D0/D1 pulled up here in case the PCB does not have discrete resistors.
    gpio_set_pin_input_high(D0);
    gpio_set_pin_input_high(D1);

    TWSR = 0;
    TWBR = (uint8_t)TWBR_VAL;
    TWCR = (1 << TWEN);
}
