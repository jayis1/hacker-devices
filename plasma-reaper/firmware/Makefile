# Makefile — PlasmaReaper Firmware
# Multi-Vector Fault Injection Toolkit
#
# Author: jayis1
# License: GPL-2.0
#
# Build requirements:
#   arm-none-eabi-gcc (GCC for ARM Cortex-M)
#   arm-none-eabi-objcopy
#
# Usage:
#   make          — build firmware.elf
#   make flash    — flash via OpenOCD (ST-Link or CMSIS-DAP)
#   make clean    — remove build artifacts

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
OBJDUMP = arm-none-eabi-objdump

# ---- Target MCU ----
MCU     = cortex-m7
FPU     = neon-fp-armv8
FLOAT_ABI = hard

# ---- Flags ----
CFLAGS  = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)
CFLAGS += -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
CFLAGS += -Os -g3 -ffunction-sections -fdata-sections
CFLAGS += -DSTM32H723xx -DPLASMA_REAPER_V1
CFLAGS += -I. -Idrivers

LDFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)
LDFLAGS += -nostdlib -nostartfiles
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -T linker.ld

# ---- Source files ----
SRCS = main.c
SRCS += drivers/hrtim_glitch.c
SRCS += drivers/fpga_if.c
SRCS += drivers/power_glitch.c
SRCS += drivers/clock_glitch.c
SRCS += drivers/em_pulse.c
SRCS += drivers/trigger.c
SRCS += drivers/uart_monitor.c
SRCS += drivers/adc_capture.c
SRCS += drivers/usb_cdc.c
SRCS += drivers/ble_if.c
SRCS += drivers/sdcard.c
SRCS += drivers/oled.c
SRCS += drivers/protocol.c

OBJS = $(SRCS:.c=.o)

# ---- Targets ----
TARGET = plasmareaper

.PHONY: all clean flash dump

all: $(TARGET).elf $(TARGET).bin $(TARGET).hex

$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

flash: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $(TARGET).bin 0x08000000 verify reset exit"

dump: $(TARGET).elf
	$(OBJDUMP) -D -S $< > $(TARGET).asm

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).asm

# ---- Dependencies ----
main.o: board.h registers.h drivers/hrtim_glitch.h drivers/fpga_if.h \
        drivers/power_glitch.h drivers/clock_glitch.h drivers/em_pulse.h \
        drivers/trigger.h drivers/uart_monitor.h drivers/adc_capture.h \
        drivers/usb_cdc.h drivers/ble_if.h drivers/sdcard.h \
        drivers/oled.h drivers/protocol.h

drivers/hrtim_glitch.o: drivers/hrtim_glitch.h registers.h board.h
drivers/fpga_if.o: drivers/fpga_if.h registers.h board.h
drivers/power_glitch.o: drivers/power_glitch.h registers.h board.h
drivers/clock_glitch.o: drivers/clock_glitch.h registers.h board.h drivers/fpga_if.h
drivers/em_pulse.o: drivers/em_pulse.h registers.h board.h
drivers/trigger.o: drivers/trigger.h registers.h board.h drivers/fpga_if.h
drivers/uart_monitor.o: drivers/uart_monitor.h registers.h board.h drivers/trigger.h
drivers/adc_capture.o: drivers/adc_capture.h registers.h board.h
drivers/usb_cdc.o: drivers/usb_cdc.h registers.h
drivers/ble_if.o: drivers/ble_if.h registers.h
drivers/sdcard.o: drivers/sdcard.h registers.h board.h
drivers/oled.o: drivers/oled.h registers.h board.h
drivers/protocol.o: drivers/protocol.h drivers/usb_cdc.h drivers/ble_if.h board.h

# ---- End of Makefile ----
# Author: jayis1