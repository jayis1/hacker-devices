# ============================================================
# Makefile for Thermal Phantom firmware
# Target: STM32H743 (Cortex-M7, bare-metal)
# Author: jayis1
# Copyright (c) 2026 jayis1
# License: GPLv3
# ============================================================

# --- Toolchain ---
CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# --- Target ---
MCU     = cortex-m7
TARGET  = thermal-phantom

# --- Flags ---
CFLAGS  = -mcpu=$(MCU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS += -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -Os -g3 -ffunction-sections -fdata-sections
CFLAGS += -DAUTHOR_NAME=\"jayis1\"
CFLAGS += -DFIRMWARE_VERSION=\"1.0.0-jayis1\"
CFLAGS += -I.

LDFLAGS = -mcpu=$(MCU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
LDFLAGS += -nostdlib -Wl,--gc-sections -Wl,-Map=$(TARGET).map
LDFLAGS += -T linker.ld

# --- Source files ---
SRCS = main.c thermal_ctl.c tec_driver.c laser_driver.c temp_sensor.c \
       trigger.c comm.c safety.c cli.c

OBJS = $(SRCS:.c=.o)

# --- Build rules ---
.PHONY: all clean flash debug

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# --- Flash via ST-Link ---
flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

# --- Debug via OpenOCD ---
debug: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(TARGET).elf verify reset exit" \
		-c "resume"

# --- Disassembly ---
disasm: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $(TARGET).disasm

# --- Size report ---
size: $(TARGET).elf
	$(SIZE) --format=berkeley $<

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex \
	      $(TARGET).map $(TARGET).disasm

# ============================================================
# Author: jayis1
# ============================================================