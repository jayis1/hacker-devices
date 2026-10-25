# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c pair.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/latency-pair.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/latency-pair.elf: $(SOURCES) board.h registers.h pair.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/latency-pair.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/latency-pair.bin: $(BUILD)/latency-pair.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_pair.c pair.c -o $(BUILD)/test-pair
	$(BUILD)/test-pair
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_wire.c pair.c wire.c -o $(BUILD)/test-wire
	$(BUILD)/test-wire
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c pair.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c pair.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
