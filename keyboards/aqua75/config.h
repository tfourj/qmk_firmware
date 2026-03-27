// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define MATRIX_ROWS 6
#define MATRIX_COLS 16

#define DIODE_DIRECTION ROW2COL

#define AQUA75_MCP23018_ADDRESS 0x20
#define AQUA75_MCP23018_COL_ORDER \
    {7, 6, 5, 4, 3, 2, 1, 0, 8, 9, 10, 11, 12, 13, 14, 15}

#define MATRIX_ROW_PINS \
    {A1, A0, D15, D14, D16, D10}

#define WS2812_BYTE_ORDER WS2812_BYTE_ORDER_GRB
