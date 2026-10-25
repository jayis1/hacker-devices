# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c health.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/entropy-sentry.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/entropy-sentry.elf: $(SOURCES) board.h registers.h health.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/entropy-sentry.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/entropy-sentry.bin: $(BUILD)/entropy-sentry.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_health.c health.c wire.c -o $(BUILD)/test-health
	$(BUILD)/test-health
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c health.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c health.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
