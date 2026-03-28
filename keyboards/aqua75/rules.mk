CUSTOM_MATRIX = lite
OS_DETECTION_ENABLE = yes
NO_USB_STARTUP_CHECK = yes

SRC += matrix.c
SRC += i2c_override.c
SRC += led_map.c
SRC += rgb_status.c
SRC += drivers/gpio/mcp23018.c
