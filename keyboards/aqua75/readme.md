# aqua75

QMK definition for the Aqua75 based on the included ZMK shield example.

This version is set up as a Pro Micro compatible QMK target with:

* 6x16 matrix
* `MCP23018` column expander on I2C address `0x20`
* `WS2812` underglow with 87 LEDs
* Default and function layers translated from `keyboards/aqua75/zmk_example/aqua75.keymap`

## Hardware notes

The included ZMK files target a `nice!nano`. This QMK keyboard is configured as a Pro Micro compatible wired target because upstream QMK in this tree does not provide a native `nice!nano` board target.

The matrix wiring was translated from the ZMK shield as:

* I2C expander columns on `MCP23018`
* Rows on `A1`, `A0`, `D15`, `D14`, `D16`, `D10`
* `WS2812` data on `D3`

## Build

```sh
make aqua75:default
```

## Flash

```sh
make aqua75:default:flash
```

## Bootloader

Enter the bootloader in one of these ways:

* Hold the top-left key while plugging the keyboard in
* Press the physical reset button on the controller
* Use the `Fn + F12` position, which is mapped to `QK_BOOT`
