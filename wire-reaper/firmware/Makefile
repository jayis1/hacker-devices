# Makefile — WireReaper firmware build
# Author: jayis1
# Copyright (c) 2026 jayis1 — GPL-2.0
#
# Builds the WireReaper firmware for STM32H743 using GCC ARM toolchain.
# Target: arm-none-eabi-gcc (GCC 12+)

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ---- MCU flags ----
MCU = -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb

# ---- Include paths ----
INCS = -I. -Idrivers

# ---- Compiler flags ----
CFLAGS = $(MCU) $(INCS) -O2 -Wall -Wextra -std=c11 -g \
         -ffunction-sections -fdata-sections -fno-common \
         -ffreestanding -nostdlib

# ---- Linker flags ----
LDFLAGS = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=wire-reaper.map \
          -nostdlib -specs=nano.specs

# ---- Source files ----
SRCS = main.c \
       drivers/board_init.c \
       drivers/i2c.c \
       drivers/spi.c \
       drivers/uart.c \
       drivers/onewire.c \
       drivers/fpga.c \
       drivers/sdram.c \
       drivers/ble_uart.c \
       drivers/usb_cdc.c \
       drivers/sdcard.c

OBJS = $(SRCS:.c=.o)

# ---- Build targets ----
TARGET = wire-reaper.elf

.PHONY: all clean flash size

all: $(TARGET) wire-reaper.bin wire-reaper.hex size

$(TARGET): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm

wire-reaper.bin: $(TARGET)
	$(OBJCOPY) -O binary $< $@

wire-reaper.hex: $(TARGET)
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET)
	$(SIZE) $(TARGET)

flash: wire-reaper.bin
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	-c "program wire-reaper.bin 0x08000000 verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET) wire-reaper.bin wire-reaper.hex wire-reaper.map

# ---- Dependencies ----
main.o: board.h registers.h
drivers/i2c.o: board.h registers.h
drivers/spi.o: board.h registers.h
drivers/uart.o: board.h registers.h
drivers/onewire.o: board.h registers.h
drivers/fpga.o: board.h registers.h
drivers/sdram.o: board.h registers.h
drivers/ble_uart.o: board.h registers.h
drivers/usb_cdc.o: board.h registers.h
drivers/sdcard.o: board.h registers.h
drivers/board_init.o: board.h registers.h