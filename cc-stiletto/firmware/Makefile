# Makefile — CC-Stiletto firmware
# Author: jayis1
# Copyright (c) 2026 jayis1. All rights reserved.
#
# Builds the CC-Stiletto bare-metal firmware for the STM32G474CEU6 using
# arm-none-eabi-gcc. Produces cc-stiletto.elf / .bin / .hex in build/.
#
# Usage:
#   make            # build
#   make flash       # flash via OpenOCD (stlink)
#   make clean

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

MCU     := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
CFLAGS  := $(MCU) -std=c11 -Os -ffreestanding -Wall -Wextra -Wno-unused-parameter \
           -ffunction-sections -fdata-sections -fno-builtin -g3 \
           -I.
LDFLAGS := $(MCU) -nostdlib -nostartfiles \
           -Wl,--gc-sections -Wl,--build-id=none \
           -T linker.ld
LDLIBS  := -lgcc -lnosys

BUILD   := build
TARGET  := cc-stiletto

SRCS := main.c pd_phy.c pd_stack.c policy_engine.c power_path.c console.c
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean flash

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) linker.ld
	@echo "  LD  $@"
	@$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	@$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	@$(OBJCOPY) -O binary $< $@
	@echo "  BIN $@"

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	@$(OBJCOPY) -O ihex $< $@
	@echo "  HEX $@"

flash: $(BUILD)/$(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program $(BUILD)/$(TARGET).hex verify reset exit 0x08000000"

clean:
	rm -rf $(BUILD)

# Author: jayis1 — 2026