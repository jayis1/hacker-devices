# ============================================================================
# Makefile — Volt Glitcher firmware build system
# Target: STM32F407VGT6 (ARM Cortex-M4F, 168 MHz)
# Toolchain: arm-none-eabi-gcc
# Copyright (c) 2026 Hacker Devices
# SPDX-License-Identifier: MIT
# ============================================================================

# ---- Toolchain ----
PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)gcc
LD      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE    := $(PREFIX)size

# ---- Project ----
PROJECT := volt-glitcher

# ---- MCU Definition ----
MCU      = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
DEFS    += -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000

# ---- Optimization ----
OPT      = -O2
DEBUG    = -g3

# ---- C Flags ----
CFLAGS   = $(MCU) $(OPT) $(DEBUG)
CFLAGS  += -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes
CFLAGS  += -Wno-unused-parameter -Wno-sign-compare
CFLAGS  += -fno-common -ffunction-sections -fdata-sections
CFLAGS  += -fno-builtin -fno-exceptions -fno-unroll-loops
CFLAGS  += $(DEFS) $(INCLUDES)

# ---- Assembler Flags ----
ASFLAGS  = $(MCU) $(OPT) $(DEBUG) $(DEFS) $(INCLUDES)

# ---- Linker Flags ----
LDFLAGS  = $(MCU) -specs=nano.specs -specs=nosys.specs
LDFLAGS += -T $(LD_SCRIPT)
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(PROJECT).map,--cref
LDFLAGS += -lm

# ---- Linker Script ----
LD_SCRIPT = stm32f407vg.ld

# ---- Include Paths ----
INCLUDES  = -I. -Idrivers

# ---- Source Files ----
C_SRCS    = main.c
C_SRCS   += drivers/fpga.c
C_SRCS   += drivers/glitch.c

# ---- Object Files ----
OBJS      = $(C_SRCS:.c=.o)

# ---- Output ----
TARGET_ELF = $(PROJECT).elf
TARGET_BIN = $(PROJECT).bin
TARGET_HEX = $(PROJECT).hex
TARGET_LST = $(PROJECT).lst

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean flash size

all: $(TARGET_BIN) $(TARGET_HEX) $(TARGET_LST)

$(TARGET_ELF): $(OBJS) $(LD_SCRIPT) Makefile
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(TARGET_HEX): $(TARGET_ELF)
	$(OBJCOPY) -O ihex $< $@

$(TARGET_LST): $(TARGET_ELF)
	$(OBJDUMP) -h -S $< > $@

# ---- Compile C sources ----
%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

# ---- Assemble S sources ----
%.o: %.S Makefile
	$(AS) $(ASFLAGS) -c -o $@ $<

# ---- Flash via ST-Link / OpenOCD ----
flash: $(TARGET_ELF)
	openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg \
		-c "program $(TARGET_ELF) verify reset exit"

flash-stlink: $(TARGET_BIN)
	st-flash write $(TARGET_BIN) 0x08000000

# ---- Clean ----
clean:
	rm -f $(OBJS) $(TARGET_ELF) $(TARGET_BIN) $(TARGET_HEX) $(TARGET_LST)
	rm -f $(PROJECT).map

# ---- Size Summary ----
size: $(TARGET_ELF)
	$(SIZE) -A $(TARGET_ELF)

# ---- Dependencies (auto-generated) ----
-include $(OBJS:.o=.d)