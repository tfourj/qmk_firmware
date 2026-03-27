// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define MATRIX_ROWS 6
#define MATRIX_COLS 16

#define AQUA75_MCP23018_ADDRESS 0x20
#define AQUA75_MCP23018_COL_ORDER \
    {7, 6, 5, 4, 3, 2, 1, 0, 8, 9, 10, 11, 12, 13, 14, 15}

#define MATRIX_ROW_PINS \
    {F6, F7, B1, B3, B2, B6}

#define RGBLIGHT_LAYERS
#define WS2812_BYTE_ORDER WS2812_BYTE_ORDER_GRB
