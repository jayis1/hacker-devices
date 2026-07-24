# -------------------------------------------------------------------------
# Makefile — Joule-Phantom smart-battery SMBus MITM implant
# Author : jayis1
# License: GPL-2.0
#
# Bare-metal ARM Cortex-M4 build for STM32L432KCU6 using arm-none-eabi-gcc.
# No CMSIS / HAL dependency — registers.h provides the register map.
# -------------------------------------------------------------------------

PROJECT    := joule-phantom
ARCH       := arm-none-eabi
CC         := $(ARCH)-gcc
OBJCOPY    := $(ARCH)-objcopy
SIZE       := $(ARCH)-size

# MCU flags
CPU        := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# Warning / debug flags
CFLAGS     := $(CPU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
              -Os -ffunction-sections -fdata-sections -g3 \
              -I. -Idrivers

LDFLAGS    := $(CPU) -T stm32l432_flash.ld \
              -Wl,--gc-sections -Wl,-Map=$(PROJECT).map -nostdlib

# Sources
SRCS       := main.c \
              drivers/smbus_port.c \
              drivers/ble_link.c \
              drivers/spoof_engine.c

OBJS       := $(SRCS:.c=.o)

# Default target
.PHONY: all clean flash size

all: $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin size

$(PROJECT).elf: $(OBJS) stm32l432_flash.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm -lnosys

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

size: $(PROJECT).elf
	$(SIZE) $(PROJECT).elf

flash: $(PROJECT).bin
	openocd -f interface/stlink.cfg -f target/stm32l4x.cfg \
	  -c "program $(PROJECT).bin verify reset exit 0x08000000"

clean:
	rm -f $(OBJS) $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin $(PROJECT).map

# Dependency tracking
-include $(OBJS:.o=.d)