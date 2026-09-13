# Credential Canary firmware build
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0-only
PREFIX ?= arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
TARGET := credential-canary
SRCS := startup.c main.c board.c protocol.c capture.c command.c
OBJS := $(SRCS:.c=.o)
CFLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -std=c11 -Os -g3 -Wall -Wextra -Werror -ffreestanding -fdata-sections -ffunction-sections -I.
LDFLAGS := -mcpu=cortex-m4 -mthumb -nostdlib -T linker.ld -Wl,--gc-sections,-Map=$(TARGET).map

.PHONY: all clean flash size check
all: $(TARGET).elf $(TARGET).bin
$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
size: $(TARGET).elf
	$(SIZE) $<
flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program $< 0x08000000 verify reset exit"
check:
	gcc -std=c11 -Wall -Wextra -Werror -ffreestanding -I. -fsyntax-only main.c board.c protocol.c capture.c command.c startup.c
clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map
