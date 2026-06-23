# ACOUSTIC-PHANTOM firmware build
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0
#
# Targets arm-none-eabi-gcc. ML models for QSPI NOR are generated
# separately using TensorFlow Lite converter (out of scope for this Makefile).

CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

MCU     := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS  := $(MCU) -mthumb -Os -g3 -Wall -Wextra -std=c11 \
           -ffreestanding -fdata-sections -ffunction-sections \
           -I. -Idrivers
LDFLAGS := $(MCU) -mthumb -nostdlib -T STM32H743VIT6_FLASH.ld \
           -Wl,--gc-sections -Wl,-Map=acoustic-phantom.map

SRCS    := main.c \
           drivers/audio_capture.c \
           drivers/beamformer.c \
           drivers/dsp_pipeline.c \
           drivers/event_detector.c \
           drivers/tflm_inference.c \
           drivers/ble_bridge.c \
           drivers/storage.c \
           drivers/tamper.c \
           drivers/ui.c

OBJS    := $(SRCS:.c=.o)
TARGET  := acoustic-phantom.elf
BIN     := acoustic-phantom.bin

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
	rm -f $(OBJS) $(TARGET) $(BIN) acoustic-phantom.map

flash: $(BIN)
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "init; reset halt; program $(BIN) verify reset exit"