# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c monitor.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/latch-ledger.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/latch-ledger.elf: $(SOURCES) board.h registers.h monitor.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/latch-ledger.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/latch-ledger.bin: $(BUILD)/latch-ledger.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_monitor.c monitor.c wire.c -o $(BUILD)/test-monitor
	$(BUILD)/test-monitor
clean:
	rm -rf $(BUILD)
