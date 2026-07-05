# Makefile — DRAM-Phantom firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build with arm-none-eabi-gcc. Target: STM32G474CEU6 (Cortex-M4F, 170 MHz).

CROSS    := arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

MCU      := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
OPT      := -O2
WARN     := -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
DEFINES  := -DSTM32G4 -DDRAMPHANTOM
INCLUDES := -I. -Idrivers

CFLAGS   := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCLUDES) -std=gnu11 -g3
LDFLAGS  := $(MCU) -Tlink.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=dramphantom.map
LDLIBS   := -lm

STARTUP  := startup_stm32g474.s

SRCS_C   := main.c \
            drivers/board_init.c \
            drivers/fpga.c \
            drivers/ddr4_snoop.c \
            drivers/rowhammer.c \
            drivers/mem_capture.c \
            drivers/covert_channel.c \
            drivers/usb_cdc.c \
            drivers/ble_uart.c \
            drivers/sdcard.c \
            drivers/oled.c

OBJS     := $(STARTUP:.s=.o) $(SRCS_C:.c=.o)

TARGET   := dramphantom.elf

.PHONY: all clean size dump

all: $(TARGET) size

$(TARGET): $(OBJS) link.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(OBJCOPY) -O ihex $@ dramphantom.hex
	$(OBJCOPY) -O binary $@ dramphantom.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET)
	$(SIZE) $(TARGET)

dump: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > dramphantom.lst

clean:
	rm -f $(OBJS) $(TARGET) dramphantom.hex dramphantom.bin dramphantom.map dramphantom.lst