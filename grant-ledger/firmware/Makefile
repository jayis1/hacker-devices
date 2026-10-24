# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c grant.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/grant-ledger.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/grant-ledger.elf: $(SOURCES) board.h registers.h grant.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/grant-ledger.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/grant-ledger.bin: $(BUILD)/grant-ledger.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_grant.c grant.c wire.c -o $(BUILD)/test-grant
	$(BUILD)/test-grant
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c grant.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
