# MagLance Firmware Makefile
# Author: jayis1
# License: GPL-2.0
#
# Build firmware for STM32G474VET6 using arm-none-eabi-gcc
# Prerequisites: arm-none-eabi-gcc, arm-none-eabi-objcopy, arm-none-eabi-size
#                OpenOCD (for flashing via ST-Link)

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
OBJDUMP = arm-none-eabi-objdump

# ---- Target MCU ----
MCU      = cortex-m4
FPU      = fpv4-sp-d16
FLOAT_ABI = hard

# ---- Flags ----
CPU_FLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS    = $(CPU_FLAGS) -std=c11 -Wall -Wextra -Wno-unused-parameter \
            -Os -g3 -ffunction-sections -fdata-sections \
            -I. -I./include

LDFLAGS   = $(CPU_FLAGS) -T linker.ld -Wl,--gc-sections \
            -Wl,-Map=maglance.map -nostdlib

LIBS      = -lc -lm -lnosys

# ---- Source files ----
SRCS = main.c \
       coil_driver.c \
       magnetometer.c \
       profile_manager.c \
       usb_iface.c

OBJS = $(SRCS:.c=.o)

# ---- Targets ----
.PHONY: all clean flash dump

all: maglance.elf maglance.bin maglance.hex

maglance.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(SIZE) $@

maglance.bin: maglance.elf
	$(OBJCOPY) -O binary $< $@

maglance.hex: maglance.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

flash: maglance.elf
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program maglance.elf verify reset exit"

dump: maglance.elf
	$(OBJDUMP) -d -S maglance.elf > maglance.lst

clean:
	rm -f $(OBJS) maglance.elf maglance.bin maglance.hex \
		maglance.map maglance.lst

# ---- Dependencies ----
main.o: main.c board.h registers.h coil_driver.h magnetometer.h \
        profile_manager.h usb_iface.h
coil_driver.o: coil_driver.c coil_driver.h board.h registers.h
magnetometer.o: magnetometer.c magnetometer.h board.h registers.h
profile_manager.o: profile_manager.c profile_manager.h board.h registers.h
usb_iface.o: usb_iface.c usb_iface.h board.h registers.h coil_driver.h \
             magnetometer.h profile_manager.h

# ---- Author ----
# jayis1