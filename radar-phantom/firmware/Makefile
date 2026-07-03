# Makefile — RadarPhantom firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build with arm-none-eabi-gcc. Target: STM32H743VIT6 (Cortex-M7, double FPU).
# The DRFM FPGA bitstream (iCE40UP5K) is generated separately with Yosys +
# nextpnr and is not part of this firmware build; it is loaded from the
# FPGA's external SPI flash at power-on.

CROSS    := arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

MCU      := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
OPT      := -O2
WARN     := -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
DEFINES  := -DSTM32H7 -DRADARPHANTOM
INCLUDES := -I. -Idrivers

CFLAGS   := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCLUDES) -std=gnu11 -g3
LDFLAGS  := $(MCU) -Tlink.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=radarphantom.map
LDLIBS   := -lm

# Startup + system stub (minimal vector table). Provided by board_init.c's
# Reset_Handler; a tiny startup assembly file supplies the vector table.
STARTUP  := startup_stm32h743.s

SRCS_C   := main.c \
            drivers/board_init.c \
            drivers/drfm.c \
            drivers/scenario.c \
            drivers/radar_cfg.c \
            drivers/lo_pll.c \
            drivers/ble_uart.c \
            drivers/usb_cdc.c \
            drivers/oled.c \
            drivers/sdcard.c \
            drivers/power.c \
            drivers/joystick.c

OBJS     := $(STARTUP:.s=.o) $(SRCS_C:.c=.o)

TARGET   := radarphantom.elf

.PHONY: all clean size dump

all: $(TARGET) size

$(TARGET): $(OBJS) link.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(OBJCOPY) -O ihex $@ radarphantom.hex
	$(OBJCOPY) -O binary $@ radarphantom.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET)
	$(SIZE) $(TARGET)

dump: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > radarphantom.lst

clean:
	rm -f $(OBJS) $(TARGET) radarphantom.hex radarphantom.bin radarphantom.map radarphantom.lst