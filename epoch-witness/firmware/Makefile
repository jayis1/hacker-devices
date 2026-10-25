# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c epoch.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/epoch-witness.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/epoch-witness.elf: $(SOURCES) board.h registers.h epoch.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/epoch-witness.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/epoch-witness.bin: $(BUILD)/epoch-witness.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_epoch.c epoch.c wire.c -o $(BUILD)/test-epoch
	$(BUILD)/test-epoch
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c epoch.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c epoch.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
