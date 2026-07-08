# tpm-phantom — Makefile
# Build system for the tpm-phantom STM32H743 firmware.
#
# Author: jayis1
# License: GPL-2.0

# ===================================================================
# Toolchain
# ===================================================================
CC       = arm-none-eabi-gcc
OBJCOPY  = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
SIZE     = arm-none-eabi-size

# ===================================================================
# Target
# ===================================================================
TARGET   = tpm-phantom
MCU      = cortex-m7
MCUFLAGS = -mcpu=$(MCU) -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

# ===================================================================
# Paths
# ===================================================================
BUILD    = build
SRCDIR   = .

# ===================================================================
# Flags
# ===================================================================
CFLAGS   = $(MCUFLAGS) -std=c11 -Wall -Wextra -Wno-unused-parameter \
           -Os -g3 -ffunction-sections -fdata-sections \
           -I. -Idrivers

LDFLAGS  = $(MCUFLAGS) -T stm32h743_flash.ld \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map \
           -nostartfiles -lc -lm

# ===================================================================
# Sources
# ===================================================================
SRCS     = main.c \
           drivers/lpc_driver.c \
           drivers/spi_tpm_driver.c \
           drivers/usb_cdc.c \
           drivers/ble_bridge.c \
           drivers/sd_capture.c \
           drivers/wire_protocol.c

OBJS     = $(patsubst %.c,$(BUILD)/%.o,$(SRCS))
DEPS     = $(OBJS:.o=.d)

# ===================================================================
# Rules
# ===================================================================
.PHONY: all clean flash size

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin size

$(BUILD):
	mkdir -p $(BUILD)/drivers

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@ -MMD -MP

$(BUILD)/$(TARGET).elf: $(OBJS) stm32h743_flash.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

size: $(BUILD)/$(TARGET).elf
	$(SIZE) $<

flash: $(BUILD)/$(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(BUILD)/$(TARGET).hex verify reset exit"

clean:
	rm -rf $(BUILD)

-include $(DEPS)