# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c drain.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/revocation-drain.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/revocation-drain.elf: $(SOURCES) board.h registers.h drain.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/revocation-drain.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/revocation-drain.bin: $(BUILD)/revocation-drain.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_drain.c drain.c -o $(BUILD)/test-drain
	$(BUILD)/test-drain
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_wire.c drain.c wire.c -o $(BUILD)/test-wire
	$(BUILD)/test-wire
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c drain.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c drain.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
