# Makefile — ZIGBEE-PHANTOM firmware
# Author: jayis1
# License: GPL-2.0
#
# Builds the CC2652R1F application-core firmware using arm-none-eabi-gcc.
# Links against TI's CC26xx driverlib (provided in TI SimpleLink SDK).
# In a minimal standalone build (without driverlib), the firmware uses the
# register-level definitions in registers.h directly.

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

# ---- Target MCU: CC2652R1F (Cortex-M4F) ----
CPU      = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   = $(CPU) -std=c11 -Os -Wall -Wextra -ffunction-sections -fdata-sections
CFLAGS  += -I. -Idrivers
CFLAGS  += -DZIGBEE_PHANTOM=1
CFLAGS  += --specs=nano.specs --specs=nosys.specs

LDFLAGS  = $(CPU) -Tzigbee-phantom.ld -Wl,--gc-sections -Wl,-Map=zigbee-phantom.map
LDLIBS   = -lc -lm -lnosys

# ---- Source files ----
SRCS = \
    main.c \
    drivers/cc2652_rf.c \
    drivers/zigbee_mac.c \
    drivers/zigbee_aps.c \
    drivers/crypto.c \
    drivers/oled.c \
    drivers/font6x8.c \
    drivers/joystick.c \
    drivers/sd_pcap.c \
    drivers/esp_backhaul.c

OBJS = $(SRCS:.c=.o)

TARGET = zigbee-phantom.elf
BIN    = zigbee-phantom.bin

.PHONY: all clean flash

all: $(TARGET) $(BIN)

$(TARGET): $(OBJS) zigbee-phantom.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	$(SIZE) $@

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(BIN) zigbee-phantom.map

flash: $(BIN)
	@echo "Flash via XDS110: tilib -f zigbee-phantom.bin"