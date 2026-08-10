# Makefile — Ember-Tap firmware build
# Author: jayis1
# License: GPL-2.0
#
# Targets the STM32G474CBT6 with arm-none-eabi-gcc.
# Produce build/embertap.elf and build/embertap.bin

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ---- MCU flags ----
MCU      = cortex-m4
FPU      = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   = -mcpu=$(MCU) -mthumb $(FPU) -std=c11 -Wall -Wextra -Os -g3
CFLAGS  += -ffreestanding -fno-common -fdata-sections -ffunction-sections
CFLAGS  += -I.

LDFLAGS  = -mcpu=$(MCU) -mthumb $(FPU) -nostdlib -nostartfiles
LDFLAGS  += -Wl,--gc-sections -Wl,--defsym=__stack=0x20008000
LDFLAGS  += -T linker.ld

# ---- Sources ----
SRCDIR   = .
DRVDIR   = drivers

SRCS     = $(SRCDIR)/main.c \
           $(DRVDIR)/pd_engine.c \
           $(DRVDIR)/power_monitor.c \
           $(DRVDIR)/usb_cdc.c \
           $(DRVDIR)/display.c \
           $(DRVDIR)/buttons.c

OBJS     = $(patsubst %.c, build/%.o, $(SRCS))

# ---- Targets ----
.PHONY: all clean flash

all: build/embertap.elf build/embertap.bin

build:
	@mkdir -p build/drivers

build/%.o: %.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/embertap.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc
	$(SIZE) $@

build/embertap.bin: build/embertap.elf
	$(OBJCOPY) -O binary $< $@

flash: build/embertap.bin
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
	  -c "program build/embertap.bin 0x08000000 verify reset exit"

clean:
	rm -rf build

# end of Makefile — author: jayis1