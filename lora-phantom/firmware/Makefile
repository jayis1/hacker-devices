# lora-phantom / Makefile
# Build system for LoRaPhantom firmware.
# Author: jayis1
# License: GPL-2.0
# Copyright (c) 2026 jayis1. All rights reserved.
#
# Target: STM32H743VIT6 (Cortex-M7 @ 480 MHz, FPU fpv5-d16)
# Toolchain: arm-none-eabi-gcc 12.2+ (newlib, arm-none-eabi-binutils)

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
OPENOCD = openocd
DFU     = dfu-util

# ---- MCU flags ----
CPU     = -mcpu=cortex-m7
FPU     = -mfpu=fpv5-d16
FLOAT   = -mfloat-abi=hard
ARCH    = $(CPU) $(FPU) $(FLOAT)

# ---- Compiler flags ----
CFLAGS  = $(ARCH) -std=c11 -Wall -Wextra -Os -g3
CFLAGS += -fdata-sections -ffunction-sections -fno-common
CFLAGS += -ffreestanding -fno-builtin
CFLAGS += -I.

# ---- Linker flags ----
LDFLAGS = $(ARCH) -nostdlib -nostartfiles
LDFLAGS += -Wl,--gc-sections -Wl,--defsym=__stack_top=0x24080000
LDFLAGS += -T linker.ld

# ---- Source files ----
SRCDIR  = .
DRVDIR  = drivers
SRCS    = $(SRCDIR)/main.c
DRVS    = $(DRVDIR)/board_init.c $(DRVDIR)/sx1262.c $(DRVDIR)/aes_hw.c \
          $(DRVDIR)/lorawan.c $(DRVDIR)/keysearch.c $(DRVDIR)/replay.c \
          $(DRVDIR)/injector.c $(DRVDIR)/fuzzer.c $(DRVDIR)/spectrum.c \
          $(DRVDIR)/ble_uart.c $(DRVDIR)/usb_cdc.c $(DRVDIR)/sdcard.c \
          $(DRVDIR)/sdram.c $(DRVDIR)/protocol.c

OBJS    = $(SRCS:.c=.o) $(DRVS:.c=.o)
TARGET  = lora-phantom
ELF     = $(TARGET).elf
BIN     = $(TARGET).bin
HEX     = $(TARGET).hex
MAP     = $(TARGET).map

# ---- Default target ----
all: $(ELF) $(BIN) $(HEX)

$(ELF): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -Wl,-Map=$(MAP) -o $@ $(OBJS) -lc -lm
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

# ---- Compile rules ----
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Linker script (generated if missing) ----
linker.ld:
	@echo "MEMORY" > linker.ld
	@echo "{" >> linker.ld
	@echo "  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K" >> linker.ld
	@echo "  DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K" >> linker.ld
	@echo "  RAM   (rwx) : ORIGIN = 0x24000000, LENGTH = 512K" >> linker.ld
	@echo "  SDRAM (rwx) : ORIGIN = 0xC0000000, LENGTH = 16M" >> linker.ld
	@echo "}" >> linker.ld
	@echo "SECTIONS" >> linker.ld
	@echo "{" >> linker.ld
	@echo "  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH" >> linker.ld
	@echo "  .text : { *(.text*) *(.rodata*) } > FLASH" >> linker.ld
	@echo "  .data : { *(.data*) } > RAM AT > FLASH" >> linker.ld
	@echo "  .bss : { *(.bss*) *(COMMON) } > RAM" >> linker.ld
	@echo "  .stack (NOLOAD) : { . = 0x80000; } > RAM" >> linker.ld
	@echo "}" >> linker.ld
	@echo "PROVIDE(_estack = 0x24080000);" >> linker.ld

# ---- Flash via ST-Link (SWD) ----
flash: $(BIN)
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(BIN) 0x08000000 verify reset exit"

# ---- Flash via USB DFU (built-in bootloader) ----
flash-usb: $(BIN)
	$(DFU) -d 0483:df11 -a 0 -s 0x08000000 -D $(BIN)

# ---- Disassembly ----
disasm: $(ELF)
	$(OBJDUMP) -d $< | less

# ---- Clean ----
clean:
	rm -f $(OBJS) $(ELF) $(BIN) $(HEX) $(MAP) linker.ld

.PHONY: all clean flash flash-usb disasm