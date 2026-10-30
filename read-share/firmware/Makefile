# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c engine.c wire.c drivers/board.c drivers/serial.c drivers/crc16.c
.PHONY: all test clean
all: $(BUILD)/read-share.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/read-share.elf: $(SOURCES) board.h registers.h engine.h wire.h drivers/crc16.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/read-share.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/read-share.bin: $(BUILD)/read-share.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_engine.c engine.c wire.c drivers/crc16.c -o $(BUILD)/test-engine
	$(BUILD)/test-engine
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c engine.c wire.c drivers/crc16.c -o $(BUILD)/fixture
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_main.c engine.c wire.c drivers/crc16.c -o $(BUILD)/test-main
	$(BUILD)/test-main
clean:
	rm -rf $(BUILD)
