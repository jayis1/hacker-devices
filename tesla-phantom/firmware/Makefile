# TeslaPhantom firmware Makefile
# Author:  jayis1
# Copyright (C) 2026 jayis1
# SPDX-License-Identifier: GPL-2.0
#
# Cross-compiles the TeslaPhantom firmware for the STM32H743 (Cortex-M7).
# Toolchain: arm-none-eabi-gcc
#
# Build:
#   make            # build tesla-phantom.elf
#   make hex        # also produce tesla-phantom.hex
#   make flash      # flash via OpenOCD (ST-Link)
#   make clean

PROJECT   := tesla-phantom
AUTHOR    := jayis1

# ---- Toolchain ---------------------------------------------------------
CC       := arm-none-eabi-gcc
OBJCOPY  := arm-none-eabi-objcopy
SIZE     := arm-none-eabi-size
OPENOCD  := openocd

# ---- Paths -------------------------------------------------------------
DRIVER_DIR := drivers

# ---- Sources -----------------------------------------------------------
SRCS := main.c \
        $(DRIVER_DIR)/board_init.c \
        $(DRIVER_DIR)/fpga_spi.c \
        $(DRIVER_DIR)/hv_pulse.c \
        $(DRIVER_DIR)/stepper.c \
        $(DRIVER_DIR)/fluxgate.c \
        $(DRIVER_DIR)/adc_capture.c \
        $(DRIVER_DIR)/sca_engine.c \
        $(DRIVER_DIR)/trigger.c \
        $(DRIVER_DIR)/scan_controller.c \
        $(DRIVER_DIR)/ble_uart.c \
        $(DRIVER_DIR)/usb_cdc.c \
        $(DRIVER_DIR)/oled.c \
        $(DRIVER_DIR)/sdcard.c \
        $(DRIVER_DIR)/config.c

OBJS := $(SRCS:.c=.o)

# ---- Flags -------------------------------------------------------------
MCUFLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

CFLAGS := $(MCUFLAGS) -std=c11 -Wall -Wextra -Os -g3 \
          -ffreestanding -fno-common -fdata-sections -ffunction-sections \
          -I.

LDFLAGS := $(MCUFLAGS) -Tlinker.ld -nostdlib -Wl,--gc-sections -Wl,-Map=$(PROJECT).map

# ---- Targets -----------------------------------------------------------
.PHONY: all hex flash clean

all: $(PROJECT).elf

hex: $(PROJECT).elf $(PROJECT).hex

$(PROJECT).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lm
	$(SIZE) $@

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

# C compile rule
%.o: %.c board.h registers.h
	$(CC) $(CFLAGS) -c -o $@ $<

flash: $(PROJECT).hex
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "init; reset halt; program $(PROJECT).hex verify; reset run; exit"

clean:
	rm -f $(OBJS) $(PROJECT).elf $(PROJECT).hex $(PROJECT).map