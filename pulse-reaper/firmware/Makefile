# Makefile — Pulse-Reaper firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build with arm-none-eabi-gcc. Target: STM32H743VIT6 (Cortex-M7, 480 MHz).
#
#   make           # build pulsereaper.elf
#   make flash     # flash via OpenOCD / ST-Link
#   make clean
#

CROSS    := arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
SIZE     := $(CROSS)size

MCU      := -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb
OPT      := -O2
WARN     := -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
DEFINES  := -DSTM32H7 -DSTM32H743xx -DPULSEREAPER
INCLUDES := -I. -Idrivers

CFLAGS   := $(MCU) $(OPT) $(WARN) $(DEFINES) $(INCLUDES) -std=gnu11 -g3
LDFLAGS  := $(MCU) -Tlink.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=pulsereaper.map
LDLIBS   := -lm

# Startup + system stub (minimal vector table)
STARTUP  := startup_stm32h743.s

SRCS_C   := main.c \
            drivers/board_init.c \
            drivers/clamp_afc.c \
            drivers/fpga_dsp.c \
            drivers/tdr_engine.c \
            drivers/protocol_detect.c \
            drivers/modbus_rtu.c \
            drivers/profibus_dp.c \
            drivers/hart_fsk.c \
            drivers/can_native.c \
            drivers/pots_dtmf.c \
            drivers/sd_capture.c \
            drivers/ble_c2.c \
            drivers/crypto.c \
            drivers/oled.c \
            drivers/power.c

OBJS     := $(STARTUP:.s=.o) $(SRCS_C:.c=.o)

.PHONY: all flash clean dump

all: pulsereaper.elf pulsereaper.bin pulsereaper.hex

pulsereaper.elf: $(OBJS) link.ld
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	$(SIZE) $@

pulsereaper.bin: pulsereaper.elf
	$(OBJCOPY) -O binary $< $@

pulsereaper.hex: pulsereaper.elf
	$(OBJCOPY) -O ihex $< $@

flash: pulsereaper.hex
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "init; reset halt; flash write_image erase pulsereaper.hex; reset run; exit"

dump: pulsereaper.elf
	$(OBJDUMP) -d -S pulsereaper.elf > pulsereaper.lst

clean:
	rm -f $(OBJS) pulsereaper.elf pulsereaper.bin pulsereaper.hex \
		pulsereaper.map pulsereaper.lst

# --- dependency tracking ---
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

-include $(SRCS_C:.c=.d)