// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "aqua75_shared.h"
#include "drivers/gpio/mcp23018.h"
#include "i2c_master.h"
#include "print.h"
#include "wait.h"

#ifdef PROTOCOL_CHIBIOS
#    include "chibios_config.h"
#    include <hal.h>
#endif

#define AQUA75_MCP_RETRY_MIN_MS 25
#define AQUA75_MCP_RETRY_MAX_MS 1000

static bool     mcp_ready            = false;
static uint16_t mcp_fail_count       = 0;
static uint16_t mcp_retry_period     = 0;
static uint32_t mcp_last_retry_timer = 0;

#ifdef CONSOLE_ENABLE
static uint32_t mcp_last_debug_log      = 0;
static bool     mcp_debug_log_seen      = false;
static uint32_t mcp_stats_timer         = 0;
static uint32_t mcp_scan_count          = 0;
static uint32_t mcp_read_count          = 0;
static uint32_t mcp_read_ok_count       = 0;
static uint32_t mcp_read_retry_count    = 0;
static uint32_t mcp_read_fail_count     = 0;
static uint32_t mcp_recover_count       = 0;
static uint32_t mcp_recover_ok_count    = 0;
static uint32_t mcp_row_change_count    = 0;
static uint16_t mcp_max_scan_elapsed_ms = 0;

static void aqua75_debug_mcp_log(const char *message, bool rate_limit) {
    if (rate_limit && mcp_debug_log_seen && timer_elapsed32(mcp_last_debug_log) < 1000) {
        return;
    }

    mcp_debug_log_seen = true;
    mcp_last_debug_log = timer_read32();
    dprintf("aqua75: mcp23018 %s, fail_count=%u, retry_period=%u\n", message, mcp_fail_count, mcp_retry_period);
}

static void aqua75_debug_mcp(const char *message) {
    aqua75_debug_mcp_log(message, true);
}

static void aqua75_debug_mcp_force(const char *message) {
    aqua75_debug_mcp_log(message, false);
}

static void aqua75_debug_mcp_row_change(uint8_t row, matrix_row_t previous, matrix_row_t current) {
    uprintf("aqua75: matrix row=%u old=0x%04X new=0x%04X diff=0x%04X\n", row, previous, current, previous ^ current);
}

static void aqua75_debug_mcp_stats(uint16_t scan_elapsed_ms) {
    mcp_scan_count++;

    if (scan_elapsed_ms > mcp_max_scan_elapsed_ms) {
        mcp_max_scan_elapsed_ms = scan_elapsed_ms;
    }

    if (timer_elapsed32(mcp_stats_timer) < 1000) {
        return;
    }

    mcp_stats_timer = timer_read32();
    uprintf("aqua75: mcp stats scans=%lu reads=%lu ok=%lu retries=%lu read_fail=%lu recover=%lu/%lu row_changes=%lu max_scan_ms=%u ready=%u fail_count=%u retry_period=%u\n",
            mcp_scan_count, mcp_read_count, mcp_read_ok_count, mcp_read_retry_count, mcp_read_fail_count, mcp_recover_ok_count, mcp_recover_count,
            mcp_row_change_count, mcp_max_scan_elapsed_ms, mcp_ready, mcp_fail_count, mcp_retry_period);

    mcp_scan_count          = 0;
    mcp_read_count          = 0;
    mcp_read_ok_count       = 0;
    mcp_read_retry_count    = 0;
    mcp_read_fail_count     = 0;
    mcp_recover_count       = 0;
    mcp_recover_ok_count    = 0;
    mcp_row_change_count    = 0;
    mcp_max_scan_elapsed_ms = 0;
}
#endif

static void unselect_rows(void) {
    const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        gpio_set_pin_input_high(row_pins[row]);
    }
}

static void select_row(uint8_t row) {
    const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;

    gpio_set_pin_output(row_pins[row]);
    gpio_write_pin_low(row_pins[row]);
}

static void i2c_recover_bus(void) {
    const pin_t scl_pin = I2C1_SCL_PIN;
    const pin_t sda_pin = I2C1_SDA_PIN;

    gpio_set_pin_output(scl_pin);
    gpio_set_pin_output(sda_pin);

    for (uint8_t i = 0; i < 10; i++) {
        gpio_write_pin_low(scl_pin);
        wait_us(10);
        gpio_write_pin_high(scl_pin);
        wait_us(10);
    }

    gpio_write_pin_low(sda_pin);
    wait_us(10);
    gpio_write_pin_high(scl_pin);
    wait_us(10);
    gpio_write_pin_high(sda_pin);
    wait_us(10);
}

static void i2c_restore_bus(void) {
#if defined(PROTOCOL_CHIBIOS) && defined(USE_GPIOV1)
    palSetLineMode(I2C1_SCL_PIN, I2C1_SCL_PAL_MODE);
    palSetLineMode(I2C1_SDA_PIN, I2C1_SDA_PAL_MODE);
#elif defined(PROTOCOL_CHIBIOS)
    palSetLineMode(I2C1_SCL_PIN, PAL_MODE_ALTERNATE(I2C1_SCL_PAL_MODE) | PAL_OUTPUT_TYPE_OPENDRAIN);
    palSetLineMode(I2C1_SDA_PIN, PAL_MODE_ALTERNATE(I2C1_SDA_PAL_MODE) | PAL_OUTPUT_TYPE_OPENDRAIN);
#else
    i2c_init();
#endif
}

static void aqua75_schedule_mcp_retry(void) {
    if (mcp_fail_count < UINT16_MAX) {
        mcp_fail_count++;
    }

    uint8_t shift = mcp_fail_count > 5 ? 5 : mcp_fail_count;
    mcp_retry_period = AQUA75_MCP_RETRY_MIN_MS << shift;

    if (mcp_retry_period > AQUA75_MCP_RETRY_MAX_MS) {
        mcp_retry_period = AQUA75_MCP_RETRY_MAX_MS;
    }

    mcp_last_retry_timer = timer_read32();
}

static bool aqua75_init_mcp23018(void) {
    mcp23018_init(AQUA75_MCP23018_ADDRESS);

    if (mcp23018_set_config(AQUA75_MCP23018_ADDRESS, mcp23018_PORTA, ALL_INPUT) &&
        mcp23018_set_config(AQUA75_MCP23018_ADDRESS, mcp23018_PORTB, ALL_INPUT)) {
#ifdef CONSOLE_ENABLE
        if (mcp_fail_count > 0) {
            aqua75_debug_mcp_force("init recovered");
        }
#endif
        mcp_fail_count = 0;
        return true;
    }

#ifdef CONSOLE_ENABLE
    aqua75_debug_mcp("init failed");
#endif
    aqua75_schedule_mcp_retry();
    return false;
}

static bool aqua75_clear_matrix(matrix_row_t current_matrix[]) {
    bool changed = false;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        if (current_matrix[row] != 0) {
#ifdef CONSOLE_ENABLE
            mcp_row_change_count++;
            aqua75_debug_mcp_row_change(row, current_matrix[row], 0);
#endif
            current_matrix[row] = 0;
            changed             = true;
        }
    }

    return changed;
}

static bool aqua75_recover_mcp23018(void) {
#ifdef CONSOLE_ENABLE
    mcp_recover_count++;
    aqua75_debug_mcp("recovering i2c bus");
#endif
    i2c_recover_bus();
    i2c_restore_bus();

    mcp23018_init(AQUA75_MCP23018_ADDRESS);

    if (mcp23018_set_config(AQUA75_MCP23018_ADDRESS, mcp23018_PORTA, ALL_INPUT) &&
        mcp23018_set_config(AQUA75_MCP23018_ADDRESS, mcp23018_PORTB, ALL_INPUT)) {
#ifdef CONSOLE_ENABLE
        mcp_recover_ok_count++;
        aqua75_debug_mcp_force("recovered");
#endif
        mcp_fail_count   = 0;
        mcp_retry_period = 0;
        aqua75_state.i2c_recovery_flash = 50;
        return true;
    }

#ifdef CONSOLE_ENABLE
    aqua75_debug_mcp("recover failed");
#endif
    aqua75_schedule_mcp_retry();
    return false;
}

static matrix_row_t read_cols(void) {
    const uint8_t col_order[MATRIX_COLS] = AQUA75_MCP23018_COL_ORDER;
    uint16_t      raw_state              = 0xFFFF;
    matrix_row_t  row_state              = 0;

    if (!mcp_ready) {
        return 0;
    }

    for (uint8_t attempt = 0; attempt < 2; attempt++) {
#ifdef CONSOLE_ENABLE
        mcp_read_count++;
        if (attempt > 0) {
            mcp_read_retry_count++;
        }
#endif
        if (mcp23018_read_pins_all(AQUA75_MCP23018_ADDRESS, &raw_state)) {
#ifdef CONSOLE_ENABLE
            mcp_read_ok_count++;
#endif
            mcp_fail_count = 0;

            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                if (!(raw_state & (1U << col_order[col]))) {
                    row_state |= ((matrix_row_t)1 << col);
                }
            }

            return row_state;
        }
    }

    mcp_ready = false;
#ifdef CONSOLE_ENABLE
    mcp_read_fail_count++;
    aqua75_debug_mcp("read failed");
#endif
    aqua75_schedule_mcp_retry();
    return 0;
}

void matrix_init_custom(void) {
#ifdef CONSOLE_ENABLE
    debug_enable    = true;
    debug_matrix    = true;
    debug_keyboard  = true;
    mcp_stats_timer = timer_read32();
#endif

    unselect_rows();
    mcp_ready = aqua75_init_mcp23018();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
#ifdef CONSOLE_ENABLE
    uint32_t scan_start = timer_read32();
#endif

    if (!mcp_ready) {
        if (mcp_retry_period > 0 && timer_elapsed32(mcp_last_retry_timer) < mcp_retry_period) {
            bool changed = aqua75_clear_matrix(current_matrix);
#ifdef CONSOLE_ENABLE
            aqua75_debug_mcp_stats(timer_elapsed32(scan_start));
#endif
            return changed;
        }

        if (mcp_fail_count >= 3) {
            mcp_ready = aqua75_recover_mcp23018();
        } else {
            mcp_ready = aqua75_init_mcp23018();
        }

        if (!mcp_ready) {
            bool changed = aqua75_clear_matrix(current_matrix);
#ifdef CONSOLE_ENABLE
            aqua75_debug_mcp_stats(timer_elapsed32(scan_start));
#endif
            return changed;
        }
    }

    bool changed = false;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        select_row(row);
        wait_us(30);

        matrix_row_t row_state = read_cols();
        if (current_matrix[row] != row_state) {
#ifdef CONSOLE_ENABLE
            mcp_row_change_count++;
            aqua75_debug_mcp_row_change(row, current_matrix[row], row_state);
#endif
            current_matrix[row] = row_state;
            changed             = true;
        }

        unselect_rows();
    }

#ifdef CONSOLE_ENABLE
    aqua75_debug_mcp_stats(timer_elapsed32(scan_start));
#endif

    return changed;
}
