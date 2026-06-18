# TRACE-REAPER firmware build
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0
#
# Targets arm-none-eabi-gcc. The FPGA bitstream is built separately with
# Yosys + nextpnr-ice40 (out of scope for this Makefile).

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

MCU     := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS  := $(MCU) -mthumb -Os -g3 -Wall -Wextra -std=c11 \
           -ffreestanding -fdata-sections -ffunction-sections \
           -I. -Idrivers
LDFLAGS := $(MCU) -mthumb -nostdlib -T STM32H743VIT6_FLASH.ld \
           -Wl,--gc-sections -Wl,-Map=trace-reaper.map

SRCS    := main.c \
           drivers/adc_frontend.c \
           drivers/fpga_corr.c \
           drivers/trace_buf.c \
           drivers/ble_bridge.c \
           drivers/crypto_model.c \
           drivers/storage.c \
           drivers/tamper.c \
           drivers/ui.c

OBJS    := $(SRCS:.c=.o)
TARGET  := trace-reaper.elf
BIN     := trace-reaper.bin

.PHONY: all clean flash

all: $(TARGET) $(BIN)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lm

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) $(BIN) trace-reaper.map

flash: $(BIN)
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "init; reset halt; program $(BIN) verify reset exit"