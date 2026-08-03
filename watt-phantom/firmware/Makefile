# Makefile — WattPhantom firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build with arm-none-eabi-gcc. Target: STM32G474QBT6 (Cortex-M4F, 170MHz).

CROSS    := arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

MCU      := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
OPT      := -O2
WARN     := -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
DEFINES  := -DSTM32G4 -DWATTPHANTOM
INCLUDES := -I. -Idrivers

CFLAGS   := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCLUDES) -std=gnu11 -g3
LDFLAGS  := $(MCU) -Tlink.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=wattphantom.map
LDLIBS   := -lm

# Startup + system stub (minimal vector table)
STARTUP  := startup_stm32g474.s

SRCS_C   := main.c \
            drivers/board_init.c \
            drivers/cc_phy.c \
            drivers/pd_controller.c \
            drivers/vbus_switch.c \
            drivers/current_monitor.c \
            drivers/covert_channel.c \
            drivers/fingerprint.c \
            drivers/ble_uart.c \
            drivers/usb_cdc.c \
            drivers/oled.c \
            drivers/fuel_gauge.c \
            drivers/usb_data_switch.c \
            drivers/cc_routing.c

OBJS     := $(STARTUP:.s=.o) $(SRCS_C:.c=.o)

TARGET   := wattphantom.elf

.PHONY: all clean size dump

all: $(TARGET) size

$(TARGET): $(OBJS) link.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(OBJCOPY) -O ihex $@ wattphantom.hex
	$(OBJCOPY) -O binary $@ wattphantom.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET)
	$(SIZE) $(TARGET)

dump: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > wattphantom.lst

clean:
	rm -f $(OBJS) $(TARGET) wattphantom.hex wattphantom.bin wattphantom.map wattphantom.lst