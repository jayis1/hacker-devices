# Makefile — 1553-Phantom firmware build
# Author: jayis1
# License: GPL-2.0
#
# Targets the STM32G474CBT6 host MCU with arm-none-eabi-gcc.
# The iCE40 codec bitstream is built separately with yosys/nextpnr (see
# the codec/ directory, not part of this firmware tree).
#
# Produce build/1553phantom.elf and build/1553phantom.bin

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ---- MCU flags ----
MCU      = cortex-m4
FPU      = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS   = -mcpu=$(MCU) -mthumb $(FPU) -std=c11 -Wall -Wextra -Os -g3
CFLAGS  += -ffreestanding -fno-common -fdata-sections -ffunction-sections
CFLAGS  += -I. -Idrivers

LDFLAGS  = -mcpu=$(MCU) -mthumb $(FPU) -nostdlib -nostartfiles
LDFLAGS  += -Wl,--gc-sections -Wl,--defsym=__stack=0x20020000
LDFLAGS  += -T linker.ld

# ---- Sources ----
SRCDIR   = .
DRVDIR   = drivers

SRCS     = $(SRCDIR)/startup.c \
           $(SRCDIR)/main.c \
           $(DRVDIR)/fpga_link.c \
           $(DRVDIR)/bc_role.c \
           $(DRVDIR)/rt_role.c \
           $(DRVDIR)/bm_role.c \
           $(DRVDIR)/mitm_role.c \
           $(DRVDIR)/capture.c \
           $(DRVDIR)/display.c \
           $(DRVDIR)/buttons.c \
           $(DRVDIR)/usb_cdc.c

OBJS     = $(patsubst %.c, build/%.o, $(SRCS))

# ---- Targets ----
.PHONY: all clean flash

all: build/1553phantom.elf build/1553phantom.bin

build:
	@mkdir -p build/drivers

build/%.o: %.c | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/1553phantom.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc
	$(SIZE) $@

build/1553phantom.bin: build/1553phantom.elf
	$(OBJCOPY) -O binary $< $@

flash: build/1553phantom.bin
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program build/1553phantom.bin 0x08000000 verify reset exit"

clean:
	rm -rf build

# ---- iCE40 codec bitstream (optional, separate toolchain) ----
.PHONY: codec
codec:
	@echo "Build the iCE40 codec with: yosys -p 'synth_ice40 -top top' codec/top.v"
	@echo "                      nextpnr-ice40 --up5k --package sg48 --json out.json"
	@echo "                      icepack out.asc build/codec.bin"