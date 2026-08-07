# Makefile — Prism-Tap Firmware
# MIPI CSI-2 / DSI Camera & Display Interface Tap & Injection Implant
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
CFLAGS += -DSTM32H730xx -DPRISM_TAP_V1
CFLAGS += -I. -Idrivers

LDFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)
LDFLAGS += -nostdlib -nostartfiles
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -T linker.ld

# ---- Source files ----
SRCS = main.c
SRCS += drivers/mipi_bridge.c
SRCS += drivers/fpga_if.c
SRCS += drivers/frame_capture.c
SRCS += drivers/frame_inject.c
SRCS += drivers/ble_if.c
SRCS += drivers/sdcard.c
SRCS += drivers/usb_cdc.c
SRCS += drivers/oled.c
SRCS += drivers/protocol.c
SRCS += drivers/power.c

OBJS = $(SRCS:.c=.o)

# ---- Targets ----
TARGET = prismetap

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
main.o: board.h registers.h drivers/mipi_bridge.h drivers/fpga_if.h \
         drivers/frame_capture.h drivers/frame_inject.h drivers/ble_if.h \
         drivers/sdcard.h drivers/usb_cdc.h drivers/oled.h \
         drivers/protocol.h drivers/power.h

drivers/mipi_bridge.o: drivers/mipi_bridge.h board.h registers.h
drivers/fpga_if.o: drivers/fpga_if.h board.h registers.h
drivers/frame_capture.o: drivers/frame_capture.h drivers/fpga_if.h \
                          drivers/sdcard.h board.h registers.h
drivers/frame_inject.o: drivers/frame_inject.h drivers/fpga_if.h board.h
drivers/ble_if.o: drivers/ble_if.h board.h registers.h
drivers/sdcard.o: drivers/sdcard.h board.h registers.h
drivers/usb_cdc.o: drivers/usb_cdc.h board.h registers.h
drivers/oled.o: drivers/oled.h board.h registers.h
drivers/protocol.o: drivers/protocol.h drivers/ble_if.h drivers/fpga_if.h \
                     drivers/frame_capture.h drivers/frame_inject.h \
                     drivers/mipi_bridge.h drivers/sdcard.h board.h
drivers/power.o: drivers/power.h board.h registers.h

# ---- End of Makefile ----
# Author: jayis1