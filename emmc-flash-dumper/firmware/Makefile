# Makefile — eMMC Flash Dumper Firmware
# Cross-compiles for STM32H743VI (Cortex-M7) using arm-none-eabi-gcc
#
# Copyright (c) 2026 Nous Research
# SPDX-License-Identifier: MIT

#===========================================================================
# Toolchain
#===========================================================================

CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size
GDB     = $(CROSS_COMPILE)gdb

#===========================================================================
# Project Configuration
#===========================================================================

TARGET      = emmc-flash-dumper
BUILD_DIR   = build
MCU         = STM32H743VI
ARCH        = cortex-m7
FPU         = fpv5-d16
FLOAT_ABI   = hard

#===========================================================================
# Source Files
#===========================================================================

C_SOURCES = \
    main.c \
    drivers/emmc.c \
    drivers/fpga.c

# Additional driver sources (stubs that will be fleshed out)
C_SOURCES_STUBS = \
    drivers/sdram.c \
    drivers/nand.c \
    drivers/spinor.c \
    drivers/usb_device.c \
    drivers/sdcard.c \
    drivers/oled.c \
    drivers/hash.c \
    drivers/power.c \
    drivers/button.c \
    drivers/led.c

#===========================================================================
# Include Paths
#===========================================================================

INCLUDES = \
    -I. \
    -Idrivers

#===========================================================================
# Compiler Flags
#===========================================================================

CPU_FLAGS = -mcpu=$(ARCH) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)

CFLAGS = $(CPU_FLAGS) \
         -O2 \
         -Wall -Wextra -Werror \
         -Wno-unused-parameter \
         -ffunction-sections \
         -fdata-sections \
         -fno-common \
         -fno-strict-aliasing \
         -ffast-math \
         -fsingle-precision-constant \
         -DSTM32H743xx \
         -DHSE_VALUE=25000000 \
         -DUSE_FULL_LL_DRIVER \
         $(INCLUDES)

ASFLAGS = $(CPU_FLAGS)

LDFLAGS = $(CPU_FLAGS) \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
          -Wl,--cref \
          -Wl,--print-memory-usage \
          -specs=nano.specs \
          -specs=nosys.specs \
          -T linker.ld

#===========================================================================
# Debug Flags
#===========================================================================

ifeq ($(DEBUG),1)
CFLAGS += -Og -g3 -DDEBUG
else
CFLAGS += -DNDEBUG
endif

#===========================================================================
# Objects
#===========================================================================

C_OBJS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
C_OBJS += $(addprefix $(BUILD_DIR)/,$(C_SOURCES_STUBS:.c=.o))

#===========================================================================
# FPGA Bitstream Embedding
#===========================================================================

FPGA_BITSTREAM_DIR = ../fpga/build
FPGA_BITSTREAM_BIN = $(FPGA_BITSTREAM_DIR)/nand_controller.bin
FPGA_BITSTREAM_H   = fpga_bitstream.h

#===========================================================================
# Build Targets
#===========================================================================

.PHONY: all clean flash debug size fpga_bitstream

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

# Generate FPGA bitstream header from binary
fpga_bitstream:
	@if [ -f "$(FPGA_BITSTREAM_BIN)" ]; then \
		echo "Generating FPGA bitstream header..."; \
		xxd -i $(FPGA_BITSTREAM_BIN) | sed 's/unsigned char/const uint8_t/' > $(FPGA_BITSTREAM_H); \
		echo "FPGA bitstream embedded: $$(wc -c < $(FPGA_BITSTREAM_BIN)) bytes"; \
	else \
		echo "WARNING: FPGA bitstream not found at $(FPGA_BITSTREAM_BIN)"; \
		echo "Creating empty placeholder..."; \
		echo "const uint8_t _binary_fpga_bitstream_bin_start[] = {0};" > $(FPGA_BITSTREAM_H); \
		echo "const uint8_t _binary_fpga_bitstream_bin_end[] = {0};" >> $(FPGA_BITSTREAM_H); \
	fi

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/drivers

# Compile C sources
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR) fpga_bitstream
	@echo "CC $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(C_OBJS)
	@echo "LD $@"
	$(CC) $(LDFLAGS) $^ -o $@
	$(SIZE) $@

# Binary
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "OBJCOPY $@"
	$(OBJCOPY) -O binary $< $@

# Intel HEX
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "OBJCOPY $@"
	$(OBJCOPY) -O ihex $< $@

# Disassembly
$(BUILD_DIR)/$(TARGET).dis: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -d $< > $@

#===========================================================================
# Flash Targets
#===========================================================================

flash: $(BUILD_DIR)/$(TARGET).bin
	@echo "Flashing via ST-Link..."
	st-flash --reset write $< 0x08000000

flash_openocd: $(BUILD_DIR)/$(TARGET).elf
	@echo "Flashing via OpenOCD..."
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
	        -c "program $< verify reset exit"

flash_jlink: $(BUILD_DIR)/$(TARGET).hex
	@echo "Flashing via J-Link..."
	JLinkExe -device $(MCU) -if SWD -speed 4000 -autoconnect 1 \
	         -CommanderScript flash.jlink

#===========================================================================
# Debug Targets
#===========================================================================

debug: $(BUILD_DIR)/$(TARGET).elf
	@echo "Starting GDB server..."
	$(GDB) -ex "target extended-remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       $<

debug_openocd:
	@echo "Starting OpenOCD GDB server on :3333..."
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &
	sleep 1
	$(GDB) -ex "target extended-remote :3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       $(BUILD_DIR)/$(TARGET).elf

#===========================================================================
# Utility Targets
#===========================================================================

size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) --format=berkeley $<

clean:
	rm -rf $(BUILD_DIR) $(FPGA_BITSTREAM_H)

distclean: clean
	rm -rf $(FPGA_BITSTREAM_DIR)

#===========================================================================
# Linker Script
#===========================================================================

linker.ld:
	@echo "Generating linker script..."
	@cat > $@ << 'LINKER_EOF'
MEMORY
{
    ITCM_RAM    (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
    DTCM_RAM    (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    AXI_SRAM    (rwx) : ORIGIN = 0x24000000, LENGTH = 512K
    SRAM1       (rwx) : ORIGIN = 0x30000000, LENGTH = 128K
    SRAM2       (rwx) : ORIGIN = 0x30020000, LENGTH = 128K
    SRAM3       (rwx) : ORIGIN = 0x30040000, LENGTH = 32K
    SRAM4       (rwx) : ORIGIN = 0x38000000, LENGTH = 64K
    SDRAM_BANK1 (rwx) : ORIGIN = 0xC0000000, LENGTH = 256M
    SDRAM_BANK2 (rwx) : ORIGIN = 0xD0000000, LENGTH = 256M
    FLASH       (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
}

ENTRY(main)

SECTIONS
{
    .vectors : {
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

    .itcm_text : {
        . = ALIGN(4);
        *(.itcm_text*)
        . = ALIGN(4);
    } > ITCM_RAM AT> FLASH
    _sitcm_text = LOADADDR(.itcm_text);
    _eitcm_text = .;

    .text : {
        . = ALIGN(4);
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
    } > FLASH

    .rodata : {
        . = ALIGN(4);
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab : { *(.ARM.extab*) } > FLASH
    .ARM.exidx : { *(.ARM.exidx*) } > FLASH

    _sidata = LOADADDR(.data);
    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM_RAM AT> FLASH

    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > DTCM_RAM

    .axiram : {
        . = ALIGN(4);
        *(.axiram*)
        . = ALIGN(4);
    } > AXI_SRAM

    .sdram_buf (NOLOAD) : {
        . = ALIGN(4);
        *(.sdram_buf*)
        . = ALIGN(4);
    } > SDRAM_BANK1

    _estack = ORIGIN(DTCM_RAM) + LENGTH(DTCM_RAM);
}
LINKER_EOF

#===========================================================================
# Help
#===========================================================================

help:
	@echo "eMMC Flash Dumper Firmware Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all            Build firmware (ELF, BIN, HEX)"
	@echo "  flash          Flash via ST-Link"
	@echo "  flash_openocd  Flash via OpenOCD"
	@echo "  flash_jlink    Flash via J-Link"
	@echo "  debug          Debug via GDB"
	@echo "  size           Print size information"
	@echo "  fpga_bitstream Generate FPGA bitstream header"
	@echo "  clean          Remove build artifacts"
	@echo "  distclean      Remove all generated files"
	@echo ""
	@echo "Variables:"
	@echo "  DEBUG=1        Build with debug symbols and -Og"
	@echo "  CROSS_COMPILE  Toolchain prefix (default: arm-none-eabi-)"
