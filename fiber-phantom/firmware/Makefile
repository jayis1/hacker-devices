# FiberPhantom Firmware Makefile
# Cross-compile for ARM Cortex-M0+ (Raspberry Pi RP2040)
# Author: jayis1
# Date:   2026-06-17

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# MCU settings
CPU     = -mcpu=cortex-m0plus
ARCH    = -mthumb $(CPU)

# Optimization
OPT     = -O2

# Include paths
INCDIR  = -I. -Idrivers

# C flags
CFLAGS  = $(ARCH) $(OPT) -Wall -Wextra -Wno-unused-parameter \
          -ffunction-sections -fdata-sections -fno-common \
          -ffreestanding -nostdlib $(INCDIR) \
          -std=c11 -D__RP2040__

# AS flags
ASFLAGS = $(ARCH)

# Linker flags
LDFLAGS = -T fiber-phantom.ld -nostartfiles -nostdlib \
          -Wl,--gc-sections -Wl,--no-wchar-size-warning \
          -Wl,--print-memory-usage

# Source files
SRCS    = main.c \
          drivers/apd_driver.c \
          drivers/cdr_driver.c \
          drivers/ble_c2_driver.c \
          drivers/sd_driver.c \
          drivers/vcsel_driver.c

# Object files
OBJS    = $(SRCS:.c=.o) startup.o

# Target
TARGET  = fiber-phantom

# SDK path (optional — for Raspberry Pi Pico SDK headers)
SDK_PATH ?=
ifneq ($(SDK_PATH),)
  INCDIR += -I$(SDK_PATH)/src/common/pico_base/include \
            -I$(SDK_PATH)/src/rp2040/hardware_regs/include \
            -I$(SDK_PATH)/src/rp2040/hardware_structs/include
  CFLAGS += -DPICO_RP2040
endif

.PHONY: all clean flash dump

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS) fiber-phantom.ld
	$(CC) $(ARCH) $(LDFLAGS) -o $@ $(OBJS) -lgcc
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

startup.o: startup.s
	$(AS) $(ASFLAGS) -o $@ $<

# Flash via OpenOCD (Raspberry Pi Debug Probe or Picoprobe)
flash: $(TARGET).elf
	openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
		-c "program $(TARGET).elf verify reset exit"

# Generate assembly listing for debugging
dump: $(TARGET).elf
	$(OBJDUMP) -d -S $(TARGET).elf > $(TARGET).lst

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).lst
	rm -f drivers/*.o