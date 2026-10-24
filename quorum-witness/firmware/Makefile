# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
CROSS ?= arm-none-eabi-
HOSTCC ?= gcc
BUILD ?= build
CFLAGS = -std=c11 -Os -g3 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -I.
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
SOURCES = main.c startup.c quorum.c wire.c drivers/board.c drivers/serial.c
.PHONY: all test clean
all: $(BUILD)/quorum-witness.bin
$(BUILD):
	mkdir -p $@
$(BUILD)/quorum-witness.elf: $(SOURCES) board.h registers.h quorum.h wire.h linker.ld | $(BUILD)
	$(CROSS)gcc $(CFLAGS) $(CPU) -nostartfiles --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD)/quorum-witness.map $(SOURCES) -o $@
	$(CROSS)size $@
$(BUILD)/quorum-witness.bin: $(BUILD)/quorum-witness.elf
	$(CROSS)objcopy -O binary $< $@
test: | $(BUILD)
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer ../tests/test_quorum.c quorum.c wire.c -o $(BUILD)/test-quorum
	$(BUILD)/test-quorum
	$(HOSTCC) $(CFLAGS) -fsanitize=undefined ../tests/test_serial.c drivers/serial.c -o $(BUILD)/test-serial
	$(BUILD)/test-serial
	$(HOSTCC) $(CFLAGS) -fsanitize=address,undefined ../tests/test_main.c quorum.c wire.c -o $(BUILD)/test-main
	$(BUILD)/test-main
	$(HOSTCC) $(CFLAGS) ../tests/fixture.c quorum.c wire.c -o $(BUILD)/fixture
clean:
	rm -rf $(BUILD)
