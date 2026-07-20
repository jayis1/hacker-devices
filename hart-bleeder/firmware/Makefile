# hart-bleeder — Makefile
# Build system for the HART Fieldbus Covert In-Line Attack Implant
#
# Author: jayis1
# License: GPL-2.0

# ── Toolchain ──────────────────────────────────────────────────
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# ── Target MCU ──────────────────────────────────────────────────
MCU     = cortex-m4
MFLOAT  = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS  = -mcpu=$(MCU) -mthumb $(MFLOAT) \
          -Os -g -Wall -Wextra -std=c11 \
          -fdata-sections -ffunction-sections \
          -I. -Idrivers

LDFLAGS = -mcpu=$(MCU) -mthumb $(MFLOAT) \
          -Tlinker.ld -Wl,--gc-sections -Wl,-Map=hart-bleeder.map \
          -nostdlib -lc -lnosys

# ── Sources ─────────────────────────────────────────────────────
SRCS = main.c hart_stack.c modem_drv.c loop_drv.c attacks.c \
       console.c storage.c c2link.c oled_drv.c board_init.c

OBJS = $(SRCS:.c=.o)

# ── Targets ─────────────────────────────────────────────────────
.PHONY: all clean flash dump size

all: hart-bleeder.elf hart-bleeder.bin hart-bleeder.hex

hart-bleeder.elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lgcc

hart-bleeder.bin: hart-bleeder.elf
	$(OBJCOPY) -O binary $< $@

hart-bleeder.hex: hart-bleeder.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

flash: hart-bleeder.bin
	openocd -f interface/stlink.cfg -f target/stm32g4x.cfg \
		-c "program hart-bleeder.bin 0x08000000 verify reset exit"

dump: hart-bleeder.elf
	$(OBJDUMP) -d -S $< > hart-bleeder.lst

size: hart-bleeder.elf
	$(SIZE) $<

clean:
	rm -f *.o *.elf *.bin *.hex *.map *.lst