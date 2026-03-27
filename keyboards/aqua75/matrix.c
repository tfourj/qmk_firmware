// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "drivers/gpio/mcp23018.h"
#include "wait.h"

static bool mcp_ready = false;

static void unselect_rows(void) {
    const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        gpio_set_pin_input(row_pins[row]);
        gpio_write_pin_low(row_pins[row]);
    }
}

static void select_row(uint8_t row) {
    const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;

    gpio_set_pin_output(row_pins[row]);
    gpio_write_pin_low(row_pins[row]);
}

static bool aqua75_init_mcp23018(void) {
    mcp23018_init(AQUA75_MCP23018_ADDRESS);

    return mcp23018_set_config(AQUA75_MCP23018_ADDRESS, mcp23018_PORTA, ALL_INPUT) &&
           mcp23018_set_config(AQUA75_MCP23018_ADDRESS, mcp23018_PORTB, ALL_INPUT);
}

static matrix_row_t read_cols(void) {
    const uint8_t col_order[MATRIX_COLS] = AQUA75_MCP23018_COL_ORDER;
    uint16_t      raw_state              = 0xFFFF;
    matrix_row_t  row_state              = 0;

    if (!mcp_ready || !mcp23018_read_pins_all(AQUA75_MCP23018_ADDRESS, &raw_state)) {
        mcp_ready = false;
        return 0;
    }

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        if (!(raw_state & (1U << col_order[col]))) {
            row_state |= ((matrix_row_t)1 << col);
        }
    }

    return row_state;
}

void matrix_init_custom(void) {
    unselect_rows();
    mcp_ready = aqua75_init_mcp23018();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;

    if (!mcp_ready) {
        mcp_ready = aqua75_init_mcp23018();
        if (!mcp_ready) {
            return false;
        }
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        select_row(row);
        wait_us(30);

        matrix_row_t row_state = read_cols();
        if (current_matrix[row] != row_state) {
            current_matrix[row] = row_state;
            changed             = true;
        }

        unselect_rows();
    }

    return changed;
}
