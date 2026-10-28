# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c engine.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/toggle-mailbox.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/toggle-mailbox.elf: $(SOURCES) board.h registers.h engine.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/toggle-mailbox.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/toggle-mailbox.bin: $(BUILD)/toggle-mailbox.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_engine.c engine.c wire.c -o $(BUILD)/test-engine
	$(BUILD)/test-engine
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c engine.c wire.c -o $(BUILD)/fixture
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_main.c engine.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
clean:
	rm -rf $(BUILD)
