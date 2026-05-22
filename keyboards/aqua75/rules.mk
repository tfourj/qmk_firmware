CUSTOM_MATRIX = lite
I2C_DRIVER_REQUIRED = yes
RAW_ENABLE = yes
CONSOLE_ENABLE = yes

SRC += matrix.c
SRC += aqua75_shared.c
SRC += aqua75_os.c
SRC += aqua75_keepalive.c
SRC += aqua75_rgb.c
SRC += led_map.c
SRC += rgb_status.c
SRC += drivers/gpio/mcp23018.c
