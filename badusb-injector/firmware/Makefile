# PHANTOM Firmware Build System
# RP2040 USB HID Emulation Payload Injector
# 
# Copyright (C) 2024 Hacker Devices
# Licensed under GPL-2.0

PICO_SDK_PATH ?= ./pico-sdk
CROSS_COMPILE ?= arm-none-eabi-

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

# Compiler flags
CFLAGS = -O2 -Wall -Wextra -Werror \
         -mcpu=cortex-m0plus -mthumb \
         -ffreestanding -nostdlib \
         -ffunction-sections -fdata-sections \
         -I$(PICO_SDK_PATH)/src/common \
         -I$(PICO_SDK_PATH)/src/rp2040/hardware_regs \
         -I$(PICO_SDK_PATH)/src/rp2_common/pico_platform \
         -I. -Idrivers

# Assembler flags
ASFLAGS = -mcpu=cortex-m0plus -mthumb

# Linker flags
LDFLAGS = -T phantom.ld -nostdlib -specs=nosys.specs \
          -Wl,--gc-sections -Wl,--wrap=malloc -Wl,--wrap=free

# Source files
SRCS = main.c \
       drivers/flash_storage.c \
       drivers/usb_hid.c \
       drivers/wifi_bridge.c

# Object files
OBJS = $(SRCS:.c=.o)

# Boot2 (stage 2 bootloader from Pico SDK)
BOOT2 = boot2.o

# Target
TARGET = phantom

# Default target
all: $(TARGET).hex $(TARGET).uf2

# Link ELF file
$(TARGET).elf: $(BOOT2) $(OBJS) phantom.ld
	$(CC) $(LDFLAGS) -o $@ $(BOOT2) $(OBJS) -lgcc

# Generate HEX file
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Generate UF2 file (for drag-and-drop flashing)
$(TARGET).uf2: $(TARGET).elf
	uf2conv $< -o $@ --family rp2040

# Compile C source
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile assembly
%.o: %.S
	$(AS) $(ASFLAGS) -o $@ $<

# Flash via OpenOCD + CMSIS-DAP
flash: $(TARGET).elf
	openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
		-c "program $< verify reset exit"

# Flash via Picoprobe
flash-pico: $(TARGET).elf
	openocd -f interface/picoprobe.cfg -f target/rp2040.cfg \
		-c "program $< verify reset exit"

# Clean build artifacts
clean:
	rm -f $(OBJS) $(BOOT2) $(TARGET).elf $(TARGET).hex $(TARGET).uf2
	rm -f drivers/*.o

# Size report
size: $(TARGET).elf
	$(SIZE) $<

# Dump symbols
symbols: $(TARGET).elf
	$(CROSS_COMPILE)nm -S $< | sort

.PHONY: all flash flash-pico clean size symbols