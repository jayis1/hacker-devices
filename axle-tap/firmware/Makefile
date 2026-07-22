# Makefile — AxleTap firmware for STM32H723ZGT6
# Author: jayis1
# License: GPL-2.0
# Date:   2026-07-22
#
# Builds the AxleTap firmware with arm-none-eabi-gcc. The bridge path is
# placed in ITCM via the .itcm section; the DMA descriptors are in DTCM.

# Toolchain
CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size
FLASH   := STM32_Programmer_CLI

# Target MCU
MCU     := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# Flags
CFLAGS  := $(MCU) -std=c11 -Wall -Wextra -Os -g3 \
           -ffunction-sections -fdata-sections \
           -ffreestanding -fno-common -fno-builtin \
           -DSTM32H7 -DAXLETAP_FW
LDFLAGS := $(MCU) -T stm32h723_flash.ld -nostartfiles -Wl,--gc-sections \
           -Wl,-Map=axletap.map -Wl,--print-memory-usage
LDLIBS  := -lc -lgcc -lnosys

# Source files
DRIVERS := drivers/phy_driver.c drivers/bridge_driver.c \
           drivers/gptp_driver.c drivers/tsn_driver.c \
           drivers/someip_driver.c drivers/ble_bridge.c \
           drivers/pcap_driver.c
SRCS    := main.c $(DRIVERS) startup_stm32h7xx.c
OBJS    := $(SRCS:.c=.o)

# Default target
all: axletap.elf

axletap.elf: $(OBJS) stm32h723_flash.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	$(SIZE) $@
	$(OBJDUMP) -h $@ > axletap.lst

# Compile rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Generate binary and hex
bin: axletap.bin
axletap.bin: axletap.elf
	$(OBJCOPY) -O binary $< $@

hex: axletap.hex
axletap.hex: axletap.elf
	$(OBJCOPY) -O ihex $< $@

# Flash via ST-Link
flash: axletap.hex
	$(FLASH) -c port=SWD -w $< -hardRst

# Clean
clean:
	rm -f $(OBJS) axletap.elf axletap.map axletap.lst axletap.bin axletap.hex

.PHONY: all bin hex flash clean