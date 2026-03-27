// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "i2c_master.h"
#include "timer.h"
#include "wait.h"

#define AQUA75_I2C_SDA_PIN D3
#define AQUA75_I2C_SCL_PIN D2
#define AQUA75_I2C_DELAY_US 5

static inline void i2c_delay(void) {
    wait_us(AQUA75_I2C_DELAY_US);
}

static inline void i2c_sda_high(void) {
    gpio_set_pin_input_high(AQUA75_I2C_SDA_PIN);
}

static inline void i2c_sda_low(void) {
    gpio_set_pin_output(AQUA75_I2C_SDA_PIN);
    gpio_write_pin_low(AQUA75_I2C_SDA_PIN);
}

static inline void i2c_scl_low(void) {
    gpio_set_pin_output(AQUA75_I2C_SCL_PIN);
    gpio_write_pin_low(AQUA75_I2C_SCL_PIN);
}

static i2c_status_t i2c_scl_high(uint16_t timeout) {
    uint16_t start = timer_read();
    gpio_set_pin_input_high(AQUA75_I2C_SCL_PIN);

    while (!gpio_read_pin(AQUA75_I2C_SCL_PIN)) {
        if ((timeout != I2C_TIMEOUT_INFINITE) && (timer_elapsed(start) > timeout)) {
            return I2C_STATUS_TIMEOUT;
        }
    }

    return I2C_STATUS_SUCCESS;
}

static i2c_status_t i2c_start_condition(uint16_t timeout) {
    i2c_sda_high();
    if (i2c_scl_high(timeout) != I2C_STATUS_SUCCESS) {
        return I2C_STATUS_TIMEOUT;
    }
    i2c_delay();
    i2c_sda_low();
    i2c_delay();
    i2c_scl_low();
    i2c_delay();
    return I2C_STATUS_SUCCESS;
}

static void i2c_stop_condition(void) {
    i2c_sda_low();
    i2c_delay();
    gpio_set_pin_input_high(AQUA75_I2C_SCL_PIN);
    i2c_delay();
    i2c_sda_high();
    i2c_delay();
}

static i2c_status_t i2c_write_bit(bool bit, uint16_t timeout) {
    if (bit) {
        i2c_sda_high();
    } else {
        i2c_sda_low();
    }

    i2c_delay();
    if (i2c_scl_high(timeout) != I2C_STATUS_SUCCESS) {
        return I2C_STATUS_TIMEOUT;
    }
    i2c_delay();
    i2c_scl_low();
    i2c_delay();
    return I2C_STATUS_SUCCESS;
}

static i2c_status_t i2c_write_byte(uint8_t data, uint16_t timeout) {
    for (uint8_t mask = 0x80; mask; mask >>= 1) {
        i2c_status_t status = i2c_write_bit(data & mask, timeout);
        if (status != I2C_STATUS_SUCCESS) {
            return status;
        }
    }

    i2c_sda_high();
    i2c_delay();
    if (i2c_scl_high(timeout) != I2C_STATUS_SUCCESS) {
        return I2C_STATUS_TIMEOUT;
    }
    i2c_delay();
    bool ack = !gpio_read_pin(AQUA75_I2C_SDA_PIN);
    i2c_scl_low();
    i2c_delay();

    return ack ? I2C_STATUS_SUCCESS : I2C_STATUS_ERROR;
}

static i2c_status_t i2c_read_byte(uint8_t *data, bool ack, uint16_t timeout) {
    uint8_t value = 0;

    i2c_sda_high();
    for (uint8_t i = 0; i < 8; i++) {
        value <<= 1;
        i2c_delay();
        if (i2c_scl_high(timeout) != I2C_STATUS_SUCCESS) {
            return I2C_STATUS_TIMEOUT;
        }
        i2c_delay();
        if (gpio_read_pin(AQUA75_I2C_SDA_PIN)) {
            value |= 1;
        }
        i2c_scl_low();
    }

    *data = value;
    return i2c_write_bit(!ack, timeout);
}

void i2c_init(void) {
    i2c_sda_high();
    gpio_set_pin_input_high(AQUA75_I2C_SCL_PIN);
    i2c_delay();
}

i2c_status_t i2c_transmit(uint8_t address, const uint8_t *data, uint16_t length, uint16_t timeout) {
    i2c_status_t status = i2c_start_condition(timeout);
    if (status != I2C_STATUS_SUCCESS) {
        return status;
    }

    status = i2c_write_byte(address | 0x00, timeout);
    for (uint16_t i = 0; i < length && status == I2C_STATUS_SUCCESS; i++) {
        status = i2c_write_byte(data[i], timeout);
    }

    i2c_stop_condition();
    return status;
}

i2c_status_t i2c_transmit_P(uint8_t address, const uint8_t *data, uint16_t length, uint16_t timeout) {
    return i2c_transmit(address, data, length, timeout);
}

i2c_status_t i2c_receive(uint8_t address, uint8_t *data, uint16_t length, uint16_t timeout) {
    i2c_status_t status = i2c_start_condition(timeout);
    if (status != I2C_STATUS_SUCCESS) {
        return status;
    }

    status = i2c_write_byte(address | 0x01, timeout);
    for (uint16_t i = 0; i < length && status == I2C_STATUS_SUCCESS; i++) {
        status = i2c_read_byte(&data[i], i + 1 < length, timeout);
    }

    i2c_stop_condition();
    return status;
}

i2c_status_t i2c_transmit_and_receive(uint8_t address, const uint8_t *tx_data, uint16_t tx_length, uint8_t *rx_data, uint16_t rx_length, uint16_t timeout) {
    i2c_status_t status = i2c_start_condition(timeout);
    if (status != I2C_STATUS_SUCCESS) {
        return status;
    }

    status = i2c_write_byte(address | 0x00, timeout);
    for (uint16_t i = 0; i < tx_length && status == I2C_STATUS_SUCCESS; i++) {
        status = i2c_write_byte(tx_data[i], timeout);
    }

    if (status == I2C_STATUS_SUCCESS) {
        status = i2c_start_condition(timeout);
    }
    if (status == I2C_STATUS_SUCCESS) {
        status = i2c_write_byte(address | 0x01, timeout);
    }
    for (uint16_t i = 0; i < rx_length && status == I2C_STATUS_SUCCESS; i++) {
        status = i2c_read_byte(&rx_data[i], i + 1 < rx_length, timeout);
    }

    i2c_stop_condition();
    return status;
}

i2c_status_t i2c_write_register(uint8_t devaddr, uint8_t regaddr, const uint8_t *data, uint16_t length, uint16_t timeout) {
    i2c_status_t status = i2c_start_condition(timeout);
    if (status != I2C_STATUS_SUCCESS) {
        return status;
    }

    status = i2c_write_byte(devaddr | 0x00, timeout);
    if (status == I2C_STATUS_SUCCESS) {
        status = i2c_write_byte(regaddr, timeout);
    }
    for (uint16_t i = 0; i < length && status == I2C_STATUS_SUCCESS; i++) {
        status = i2c_write_byte(data[i], timeout);
    }

    i2c_stop_condition();
    return status;
}

i2c_status_t i2c_write_register16(uint8_t devaddr, uint16_t regaddr, const uint8_t *data, uint16_t length, uint16_t timeout) {
    uint8_t prefix[2] = {(uint8_t)(regaddr >> 8), (uint8_t)(regaddr & 0xFF)};
    i2c_status_t status = i2c_transmit(devaddr, prefix, sizeof(prefix), timeout);
    if (status != I2C_STATUS_SUCCESS || !length) {
        return status;
    }
    return i2c_transmit(devaddr, data, length, timeout);
}

i2c_status_t i2c_read_register(uint8_t devaddr, uint8_t regaddr, uint8_t *data, uint16_t length, uint16_t timeout) {
    return i2c_transmit_and_receive(devaddr, &regaddr, 1, data, length, timeout);
}

i2c_status_t i2c_read_register16(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length, uint16_t timeout) {
    uint8_t regbuf[2] = {(uint8_t)(regaddr >> 8), (uint8_t)(regaddr & 0xFF)};
    return i2c_transmit_and_receive(devaddr, regbuf, sizeof(regbuf), data, length, timeout);
}

i2c_status_t i2c_ping_address(uint8_t address, uint16_t timeout) {
    i2c_status_t status = i2c_start_condition(timeout);
    if (status != I2C_STATUS_SUCCESS) {
        return status;
    }

    status = i2c_write_byte(address | 0x00, timeout);
    i2c_stop_condition();
    return status;
}
