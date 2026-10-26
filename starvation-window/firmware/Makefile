# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c isolation.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/starvation-window.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/starvation-window.elf: $(SOURCES) board.h registers.h isolation.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/starvation-window.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/starvation-window.bin: $(BUILD)/starvation-window.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_engine.c isolation.c wire.c -o $(BUILD)/test-engine
	$(BUILD)/test-engine
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_fairness.c isolation.c -o $(BUILD)/test-fairness
	$(BUILD)/test-fairness
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c isolation.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c isolation.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
