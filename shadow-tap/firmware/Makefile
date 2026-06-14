# ShadowTap Firmware Makefile
# Cross-compile for ARM Cortex-M7 (i.MX RT1062)

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size

# MCU settings
CPU     = -mcpu=cortex-m7
FPU     = -mfpu=fpv5-d16 -mfloat-abi=hard
ARCH    = -mthumb $(CPU) $(FPU)

# Optimization
OPT     = -O2

# Include paths
INCDIR  = -I. -Idrivers

# C flags
CFLAGS  = $(ARCH) $(OPT) -Wall -Wextra -Wno-unused-parameter \
          -ffunction-sections -fdata-sections -fno-common \
          -ffreestanding -nostdlib $(INCDIR)

# AS flags
ASFLAGS = $(ARCH)

# Linker flags
LDFLAGS = -T shadow-tap.ld -nostartfiles -nostdlib \
          -Wl,--gc-sections -Wl,--no-wchar-size-warning \
          -Wl,--print-memory-usage

# Source files
SRCS    = main.c \
          drivers/enet_driver.c \
          drivers/ble_c2_driver.c

# Object files
OBJS    = $(SRCS:.c=.o) startup.o

# Target
TARGET  = shadow-tap

# SDK path (optional — for NXP MCUXpresso SDK headers)
SDK_PATH ?=
ifneq ($(SDK_PATH),)
  INCDIR += -I$(SDK_PATH)/CMSIS/Core/Include \
            -I$(SDK_PATH)/devices/MIMXRT1062
  CFLAGS += -DCPU_MIMXRT1062DVL6B
endif

.PHONY: all clean flash

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS) shadow-tap.ld
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

flash: $(TARGET).hex
	openocd -f interface/cmsis-dap.cfg -f target/imxrt1062.cfg \
		-c "program $(TARGET).hex verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex
	rm -f drivers/*.o