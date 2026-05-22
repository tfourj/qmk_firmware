// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define MATRIX_ROWS 6
#define MATRIX_COLS 16
#define IGNORE_ATOMIC_BLOCK

#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN GP2
#define I2C1_SCL_PIN GP3
#define I2C1_CLOCK_SPEED 100000

#define AQUA75_MCP23018_ADDRESS 0x20
#define AQUA75_MCP23018_COL_ORDER \
    {7, 6, 5, 4, 3, 2, 1, 15, 14, 13, 12, 11, 10, 9, 8, 0}

#define MATRIX_ROW_PINS \
    {GP4, GP5, GP6, GP7, GP8, GP9}

#define WS2812_BYTE_ORDER WS2812_BYTE_ORDER_GRB
