# Makefile — GNSS-Phantom firmware build
#
# Target: STM32H743VI (Cortex-M7, 480 MHz)
# Toolchain: arm-none-eabi-gcc
# Author: jayis1
# License: Proprietary — Authorized Security Research Use Only

CROSS_COMPILE ?= arm-none-eabi-
CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE    := $(CROSS_COMPILE)size
GDB     := $(CROSS_COMPILE)gdb

# MCU flags
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
MCU     += -ffunction-sections -fdata-sections -fno-common -fno-exceptions

CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter -Os -g3
CFLAGS  += -I. -Idrivers
CFLAGS  += -DGNSS_PHANTOM_FIRMWARE

LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=gnss-phantom.map
LDFLAGS += -Wl,--print-memory-usage

# Source files
SRCS = main.c \
       drivers/si4463.c \
       drivers/gnss_engine.c \
       drivers/c2_protocol.c

OBJS = $(SRCS:.c=.o)

TARGET = gnss-phantom

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

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map

# Flash via OpenOCD + ST-Link V3
flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	    -c "program $(TARGET).hex verify reset exit"

# Debug with GDB + OpenOCD
debug: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &
	$(GDB) $(TARGET).elf -ex "target remote :3333" -ex "load" -ex "monitor reset halt"