# aperture-phantom / firmware / Makefile
#
# Builds the AperturePhantom control-MCU firmware for STM32H743 (Cortex-M7).
# Uses arm-none-eabi-gcc. The FPGA gateware is a separate Lattice Radiant
# project (see fpga_gateware.md) and is not built here.
#
# Author: jayis1
# License: GPL-2.0

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

MCU     := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
CFLAGS  := $(MCU) -std=c11 -Os -Wall -Wextra -ffunction-sections -fdata-sections \
           -I. -ffreestanding -fno-builtin
LDFLAGS := $(MCU) -Tlinker.ld -Wl,--gc-sections -Wl,-Map=aperture-phantom.map \
           -nostdlib -Wl,--print-memory-usage

DRIVERS := drivers/board_init.c drivers/fpga.c drivers/sdram.c drivers/sdcard.c \
           drivers/i2c.c drivers/uart.c drivers/ble_uart.c drivers/usb_cdc.c \
           drivers/onewire.c

OBJS    := main.o $(patsubst %.c,%.o,$(DRIVERS))

all: aperture-phantom.elf aperture-phantom.hex aperture-phantom.bin

main.o: main.c board.h registers.h
	$(CC) $(CFLAGS) -c main.c -o main.o

drivers/%.o: drivers/%.c board.h registers.h
	$(CC) $(CFLAGS) -c $< -o $@

aperture-phantom.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

size: aperture-phantom.elf
	$(SIZE) aperture-phantom.elf

clean:
	rm -f *.o drivers/*.o aperture-phantom.elf aperture-phantom.hex \
	      aperture-phantom.bin aperture-phantom.map

.PHONY: all clean size