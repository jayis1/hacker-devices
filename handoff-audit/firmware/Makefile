# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c handoff.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/handoff-audit.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/handoff-audit.elf: $(SOURCES) board.h registers.h handoff.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/handoff-audit.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/handoff-audit.bin: $(BUILD)/handoff-audit.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_handoff.c handoff.c wire.c -o $(BUILD)/test-handoff
	$(BUILD)/test-handoff
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c handoff.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c handoff.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
