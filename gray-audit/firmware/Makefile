# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c audit.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/gray-audit.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/gray-audit.elf: $(SOURCES) board.h registers.h audit.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/gray-audit.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/gray-audit.bin: $(BUILD)/gray-audit.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_audit.c audit.c wire.c -o $(BUILD)/test-audit
	$(BUILD)/test-audit
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
clean:
	rm -rf $(BUILD)
