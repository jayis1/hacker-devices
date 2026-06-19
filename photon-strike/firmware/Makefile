# Makefile — PhotonStrike firmware (STM32H757 dual-core)
# Author: jayis1
# License: GPL-2.0
#
# Builds the M7 and M4 images and concatenates them into a single .hex
# suitable for DFU flashing. The M4 image is linked at 0x080E0000.

CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE    = $(CROSS)size

CFLAGS  = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard \
          -O2 -g -Wall -Wextra -std=c11 \
          -ffreestanding -fdata-sections -ffunction-sections \
          -I. -Idrivers -Ithird_party/cmsis -Ithird_party/fatfs
LDFLAGS = $(CFLAGS) -T photonstrike.ld -Wl,--gc-sections -Wl,-Map=photonstrike.map -nostdlib

# M7 sources
M7_SRCS = main.c fpga_bitstream.c \
          drivers/gpio.c drivers/laser.c drivers/trigger.c \
          drivers/fpga.c drivers/mems.c drivers/dfa.c \
          drivers/ble_uart.c drivers/sdmmc.c drivers/usb_dev.c \
          drivers/timing.c \
          third_party/cmsis/startup_stm32h757xx.c \
          third_party/fatfs/ff.c third_party/fatfs/diskio.c \
          third_party/stm32_usb/usb_cdc.c

# M4 sources (linked at 0x080E0000)
M4_SRCS = m4_core.c
M4_LDFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
             -O2 -g -Wall -std=c11 -ffreestanding -nostdlib \
             -T m4_link.ld -Wl,--gc-sections -Wl,-Map=m4.map

OBJS_M7 = $(M7_SRCS:.c=.o)
OBJS_M4 = $(M4_SRCS:.c=.o)

.PHONY: all clean flash fpga

all: photonstrike_m7.elf photonstrike_m4.elf photonstrike.hex

photonstrike_m7.elf: $(OBJS_M7) photonstrike.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS_M7) -lc -lm
	$(SIZE) $@

photonstrike_m4.elf: $(OBJS_M4) m4_link.ld
	$(CC) $(M4_LDFLAGS) -o $@ $(OBJS_M4) -lc -lm
	$(SIZE) $@

photonstrike.hex: photonstrike_m7.elf photonstrike_m4.elf
	$(OBJCOPY) -O ihex photonstrike_m7.elf photonstrike_m7.hex
	$(OBJCOPY) -O ihex photonstrike_m4.elf photonstrike_m4.hex
	# Concatenate with the M4 image at 0x080E0000 (srec_cat or hexmerge)
	srec_cat photonstrike_m7.hex -Intel \
	         photonstrike_m4.hex -Intel -offset 0x00E00000 \
	         -o photonstrike.hex -Intel

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

flash: photonstrike.hex
	STM32_Programmer_CLI -c port=usb -d photonstrike.hex

# FPGA bitstream (yosys + nextpnr-ice40 + icepack)
fpga:
	cd fpga && yosys -p "synth_ice40 -top photonstrike -json photonstrike.json" photonstrike.v
	cd fpga && nextpnr-ice40 --up5k --package sg48 --json photonstrike.json --asc photonstrike.asc
	cd fpga && icepack photonstrike.asc photonstrike.bit
	xxd -i fpga/photonstrike.bit > fpga_bitstream_generated.c

clean:
	rm -f $(OBJS_M7) $(OBJS_M4) *.o *.elf *.hex *.map drivers/*.o