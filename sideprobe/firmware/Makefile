# Makefile — SideProbe firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build with arm-none-eabi-gcc. Target: STM32H743VIT6 (Cortex-M7, double FPU).

CROSS    := arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

MCU      := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
OPT      := -O2
WARN     := -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
DEFINES  := -DSTM32H7 -DSIDEPROBE
INCLUDES := -I. -Idrivers

CFLAGS   := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCLUDES) -std=gnu11 -g3
LDFLAGS  := $(MCU) -Tlink.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=sideprobe.map
LDLIBS   := -lm

# Startup + system stub (minimal vector table, see below)
STARTUP  := startup_stm32h743.s

SRCS_C   := main.c \
            drivers/board_init.c \
            drivers/adc.c \
            drivers/em_probe.c \
            drivers/fpga.c \
            drivers/cpa.c \
            drivers/target_io.c \
            drivers/ble_uart.c \
            drivers/usb_cdc.c \
            drivers/sdcard.c \
            drivers/oled.c

OBJS     := $(STARTUP:.s=.o) $(SRCS_C:.c=.o)

TARGET   := sideprobe.elf

.PHONY: all clean size dump

all: $(TARGET) size

$(TARGET): $(OBJS) link.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(OBJCOPY) -O ihex $@ sideprobe.hex
	$(OBJCOPY) -O binary $@ sideprobe.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

size: $(TARGET)
	$(SIZE) $(TARGET)

dump: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > sideprobe.lst

clean:
	rm -f $(OBJS) $(TARGET) sideprobe.hex sideprobe.bin sideprobe.map sideprobe.lst