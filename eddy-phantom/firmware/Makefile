# eddy-phantom — Makefile
# Build system for the Electromagnetic Emanation Keyboard Reconnaissance Device
#
# Author: jayis1
# License: GPL-2.0

# ── Toolchain ──────────────────────────────────────────
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# ── Target ─────────────────────────────────────────────
MCU     = cortex-m7
MFLOAT  = -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS  = -mcpu=$(MCU) -mthumb $(MFLOAT) \
          -Os -g -Wall -Wextra -std=c11 \
          -fdata-sections -ffunction-sections \
          -I. -Idrivers

LDFLAGS = -mcpu=$(MCU) -mthumb $(MFLOAT) \
          -Tlink.ld -Wl,--gc-sections -Wl,-Map=eddy-phantom.map \
          -nostdlib -lc -lnosys

# ── Sources ────────────────────────────────────────────
SRCS = main.c
DRIVERS = drivers/board_init.c drivers/adc_capture.c drivers/dsp_pipeline.c \
          drivers/classifier.c drivers/probe_array.c drivers/sdcard.c \
          drivers/sdram.c drivers/qspi_flash.c drivers/ble_c2.c \
          drivers/oled.c drivers/profile_mgr.c
ASM = startup_stm32h743.s

OBJS = $(SRCS:.c=.o) $(DRIVERS:.c=.o) $(ASM:.s=.o)

# ── Targets ────────────────────────────────────────────
.PHONY: all clean flash dump

all: eddy-phantom.elf eddy-phantom.bin eddy-phantom.hex

eddy-phantom.elf: $(OBJS) link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lgcc

eddy-phantom.bin: eddy-phantom.elf
	$(OBJCOPY) -O binary $< $@

eddy-phantom.hex: eddy-phantom.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

flash: eddy-phantom.bin
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program eddy-phantom.bin 0x08000000 verify reset exit"

dump: eddy-phantom.elf
	$(OBJDUMP) -d -S $< > eddy-phantom.lst

size: eddy-phantom.elf
	$(SIZE) $<

clean:
	rm -f *.o drivers/*.o *.elf *.bin *.hex *.map *.lst