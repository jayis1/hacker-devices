# CAN Bus Infiltrator — Firmware Makefile
# Cross-compile for ARM Cortex-M4 (STM32F407)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# MCU settings
MCU      = cortex-m4
FPU      = fpv4-sp-d16
FLOAT_ABI = hard

# CPU flags
CPU_FLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)

# Assembler flags
ASFLAGS = $(CPU_FLAGS)

# Compiler flags
CFLAGS  = $(CPU_FLAGS)
CFLAGS += -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wno-unused-parameter -Wshadow -Wredundant-decls
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += -ffreestanding -fno-builtin -nostdlib
CFLAGS += -O2 -g3
CFLAGS += -I. -Idrivers
CFLAGS += -DSTM32F407xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000
CFLAGS += -DSYSCLK_FREQ=168000000 -DAPB1_FREQ=42000000 -DAPB2_FREQ=84000000

# Linker flags
LDFLAGS  = -T stm32f407vg.ld
LDFLAGS += --gc-sections --print-memory-usage
LDFLAGS += -Map=$(TARGET).map

# Source files
TARGET   = can_infiltrator

SRCDIR   = .
DRVDIR   = drivers

SOURCES  = $(SRCDIR)/main.c
SOURCES += $(DRVDIR)/can.c
SOURCES += $(DRVDIR)/ble_spi.c
SOURCES += $(DRVDIR)/sdcard.c
SOURCES += $(DRVDIR)/i2c_eeprom.c
SOURCES += $(DRVDIR)/ws2812b.c
SOURCES += $(DRVDIR)/usb_cdc.c
SOURCES += startup_stm32f407xx.s

OBJECTS  = $(SOURCES:.c=.o)
OBJECTS += $(SOURCES:.s=.o)

# Default target
all: $(TARGET).bin $(TARGET).hex $(TARGET).elf
	$(SIZE) $(TARGET).elf

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# Link
$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ -lgcc

# Generate binary
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Generate hex
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Generate disassembly
$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $@

# Flash via ST-Link
flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $< verify reset exit 0x08004000"

# Flash via DFU
dfu: $(TARGET).bin
	dfu-util -d 0483:df11 -a 0 -s 0x08004000 -D $<

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map $(TARGET).lst

.PHONY: all clean flash dfu