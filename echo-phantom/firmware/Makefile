# Makefile — ECHO-Phantom Firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build:  make
# Flash:  make flash
# Clean:  make clean
# Size:   make size
#
# Toolchain: arm-none-eabi-gcc (GNU Arm Embedded)

# ========================================================================
#  Toolchain
# ========================================================================

CC       = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ========================================================================
#  Target MCU
# ========================================================================

MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
DEFS     = -DSTM32H743xx -DUSE_FULL_LL_DRIVER
INCLUDES = -I. -Idrivers

# ========================================================================
#  Compiler Flags
# ========================================================================

CFLAGS   = $(MCU) $(DEFS) $(INCLUDES) -Os -Wall -Wextra -std=c11 \
           -fdata-sections -ffunction-sections -ffreestanding -fno-common \
           -g3 -ggdb3

LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,--print-memory-usage \
           -Wl,-Map=echo-phantom.map,--cref -nostdlib -nostartfiles

LIBS     = -lc -lm -lgcc

# ========================================================================
#  Source Files
# ========================================================================

SRCS = main.c
DRVS = drivers/board_init.c \
       drivers/sai_audio.c \
       drivers/format_detect.c \
       drivers/audio_dsp.c \
       drivers/capture.c \
       drivers/inject.c \
       drivers/ble_c2.c \
       drivers/usb_cdc.c \
       drivers/sdcard.c \
       drivers/sdram.c \
       drivers/flash_ops.c

OBJS = $(SRCS:.c=.o) $(DRVS:.c=.o)

# ========================================================================
#  Targets
# ========================================================================

TARGET = echo-phantom

.PHONY: all clean flash size dump

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(SIZE) $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

flash: $(TARGET).hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(TARGET).elf verify reset exit"

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

dump: $(TARGET).elf
	$(OBJDUMP) -d -S $(TARGET).elf > $(TARGET).asm

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin \
	      $(TARGET).map $(TARGET).asm