# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c quarantine.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/reset-quarantine.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/reset-quarantine.elf: $(SOURCES) board.h registers.h quarantine.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/reset-quarantine.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/reset-quarantine.bin: $(BUILD)/reset-quarantine.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_quarantine.c quarantine.c wire.c -o $(BUILD)/test-quarantine
	$(BUILD)/test-quarantine
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c quarantine.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c quarantine.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
