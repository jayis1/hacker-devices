# Powerline-Reaper firmware Makefile
#
# Author: jayis1
# License: GPL-2.0
#
# Targets:
#   make        - build powerline-reaper.elf + .bin
#   make flash  - openocd flash via STLINK
#   make clean
#   make size

PROJECT   = powerline-reaper
TARGET    = $(PROJECT)

# ---- Toolchain (arm-none-eabi-gcc) ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
OPENOCD = openocd

# ---- MCU / flags ----
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
           -Os -g3 -ffunction-sections -fdata-sections \
           -fstack-protector-strong -fno-common \
           -I. -Idrivers \
           -DSTM32H743xx -DUSE_FULL_LL_DRIVER=0
LDFLAGS  = $(MCU) -T stm32h743_flash.ld \
           -Wl,--gc-sections -Wl,--print-memory-usage -Wl,-Map=$(PROJECT).map \
           -nostdlib -Wl,--no-undefined

# ---- Sources ----
SRCS = main.c \
       drivers/qca7420.c \
       drivers/plc_capture.c \
       drivers/plc_topo.c \
       drivers/nmk_attack.c \
       drivers/pbkdf2.c \
       drivers/sha256.c \
       drivers/chacha20poly1305.c \
       drivers/ble_uart.c \
       drivers/sd_pcap.c \
       drivers/fatfs.c \
       drivers/usb_cdc_ecm.c \
       drivers/flash.c \
       drivers/fuel_gauge.c

OBJS = $(SRCS:.c=.o)

# ---- Rules ----
all: $(TARGET).bin

$(TARGET).elf: $(OBJS) stm32h743_flash.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

flash: $(TARGET).bin
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32h7.cfg \
	   -c "program $(TARGET).bin 0x08000000 verify reset exit"

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all clean flash size