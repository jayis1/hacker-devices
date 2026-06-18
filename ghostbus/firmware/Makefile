# GHOSTBUS — Covert SWD/JTAG Hardware Debug Implant
# Firmware Build System
#
# Cross-compilation Makefile for STM32H563ZIT6
# Requires: arm-none-eabi-gcc, openocd
#
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC       = $(CROSS_COMPILE)gcc
LD       = $(CROSS_COMPILE)ld
OBJCOPY  = $(CROSS_COMPILE)objcopy
OBJDUMP  = $(CROSS_COMPILE)objdump
SIZE     = $(CROSS_COMPILE)size
GDB      = $(CROSS_COMPILE)gdb

# Project
PROJECT  = ghostbus
TARGET   = $(PROJECT)

# Paths
BUILD_DIR = build
SRC_DIR  = .
DRV_DIR  = $(SRC_DIR)/drivers

# Sources
SRCS  = $(SRC_DIR)/main.c
SRCS += $(DRV_DIR)/swd_driver.c
SRCS += $(DRV_DIR)/jtag_driver.c
SRCS += $(DRV_DIR)/discovery_driver.c
SRCS += $(DRV_DIR)/ble_bridge.c
SRCS += $(DRV_DIR)/crypto_driver.c

# Objects
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Includes
INCLUDES  = -I$(SRC_DIR)
INCLUDES += -I$(DRV_DIR)

# Architecture: STM32H563ZIT6 (Cortex-M33, FPU, TrustZone)
ARCH_FLAGS  = -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv5-sp-d16
ARCH_FLAGS += -mcmse

# CPU frequency
DEFINES  = -DHSI_VALUE=64000000
DEFINES += -DPLATFORM_STM32H5

# Optimization
OPT = -Os

# Compiler flags
CFLAGS  = $(ARCH_FLAGS) $(OPT) $(DEFINES) $(INCLUDES)
CFLAGS += -Wall -Wextra -Wpedantic -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-common -fno-exceptions -fno-rtti
CFLAGS += -std=c11
CFLAGS += -MMD -MP

# Linker flags
LDFLAGS  = $(ARCH_FLAGS) -T$(SRC_DIR)/stm32h563_flash.ld
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -specs=nano.specs -specs=nosys.specs
LDFLAGS += -lm

# Libraries
LIBS = -lc -lnosys -lm

# Default target: build firmware binary
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(SRC_DIR)/stm32h563_flash.ld
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/drivers/%.o: $(DRV_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Binary output
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Assembly listing
lst: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -S --disassemble $< > $(BUILD_DIR)/$(TARGET).lst

# Display size
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

# Flash via OpenOCD (ST-Link V3)
flash: $(BUILD_DIR)/$(TARGET).bin
	openocd -f interface/stlink-dap.cfg \
	        -f target/stm32h5x.cfg \
	        -c "program $(BUILD_DIR)/$(TARGET).bin 0x08000000 verify reset exit"

# Flash via DFU (USB DFU bootloader)
dfu: $(BUILD_DIR)/$(TARGET).bin
	dfu-util -a 0 -D $(BUILD_DIR)/$(TARGET).bin -s 0x08000000:leave

# Debug with GDB
debug: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink-dap.cfg \
	        -f target/stm32h5x.cfg &
	sleep 1
	$(GDB) -ex "target remote localhost:3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       -ex "continue" \
	       $(BUILD_DIR)/$(TARGET).elf

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Dependencies
-include $(OBJS:.o=.d)

# Phony targets
.PHONY: all clean flash dfu debug lst size