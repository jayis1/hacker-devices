# lumen-tap/firmware/Makefile
# Build LumenTap firmware for STM32H743VI with arm-none-eabi-gcc.
#
# Author: jayis1
# Copyright (c) 2026 jayis1 — MIT License

PROJECT    := lumentap
MCU        := cortex-m7
MCUFLAGS   := -mcpu=$(MCU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

CC         := arm-none-eabi-gcc
OBJCOPY    := arm-none-eabi-objcopy
SIZE       := arm-none-eabi-size
OPENOCD    := openocd

# CMSIS-DSP path (adjust to your toolchain)
CMSIS_DIR  ?= /usr/arm/CMSIS-DSP
CMSIS_INC  := -I$(CMSIS_DIR)/Include -I$(CMSIS_DIR)/PrivateInclude

CFLAGS     := $(MCUFLAGS) -std=c11 -O2 -Wall -Wextra -g \
             -ffunction-sections -fdata-sections -fno-common \
             -ffreestanding -fno-builtin \
             -I. -Idrivers $(CMSIS_INC) \
             -DSTM32H743xx -DARM_MATH_CM7 -D__FPU_PRESENT=1
LDFLAGS    := $(MCUFLAGS) -nostdlib -nostartfiles \
             -Wl,--gc-sections -Wl,-Map=$(PROJECT).map \
             -T linker.ld
LDLIBS     := -lc -lm -lgcc

# Sources
SRCS       := main.c \
              drivers/dsp.c \
              drivers/laser.c \
              drivers/audio.c \
              drivers/sdlog.c

OBJS       := $(SRCS:.c=.o)

.PHONY: all clean flash

all: $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PROJECT).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(SIZE) $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(PROJECT).elf
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32h7x.cfg \
	  -c "program $(PROJECT).elf verify reset exit"

clean:
	rm -f $(OBJS) $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex $(PROJECT).map