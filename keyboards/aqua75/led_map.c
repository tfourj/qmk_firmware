// Copyright 2026 TfourJ
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "led_map.h"

static const uint8_t aqua75_matrix_to_led_map[MATRIX_ROWS][MATRIX_COLS] = {
    {AQUA75_NO_LED, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, AQUA75_NO_LED},
    {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
    {30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45},
    {46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, AQUA75_NO_LED},
    {61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, AQUA75_NO_LED},
    {76, 77, 78, AQUA75_NO_LED, AQUA75_NO_LED, AQUA75_NO_LED, 79, AQUA75_NO_LED, AQUA75_NO_LED, 80, 81, 82, 83, 84, 85, AQUA75_NO_LED},
};

uint8_t aqua75_matrix_to_led(uint8_t row, uint8_t col) {
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return AQUA75_NO_LED;
    }

    return aqua75_matrix_to_led_map[row][col];
}
