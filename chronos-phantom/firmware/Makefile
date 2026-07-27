# Makefile — Chronos-Phantom firmware build
#
# Builds the STM32H753 firmware for Chronos-Phantom, the IEEE 1588 PTP /
# NTP time-synchronization attack platform.
#
# Author: jayis1
# License: GPL-2.0

# --------------------------------------------------------------------
#  Toolchain
# --------------------------------------------------------------------
CROSS    = arm-none-eabi-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump
SIZE     = $(CROSS)size

# --------------------------------------------------------------------
#  MCU & flags
# --------------------------------------------------------------------
MCU      = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
CFLAGS   = $(MCU) -std=c11 -Wall -Wextra -Wno-unused-parameter \
            -O2 -g3 -ffunction-sections -fdata-sections \
            -ffreestanding -nostdlib -I.
LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=chronos-phantom.map

# --------------------------------------------------------------------
#  Sources
# --------------------------------------------------------------------
SRCS = main.c \
       ptp_engine.c \
       ntp_engine.c \
       eth_bridge.c \
       tcxo_drvr.c \
       skew_gen.c \
       covert_codec.c \
       ble_link.c \
       imux_tamper.c

OBJS = $(SRCS:.c=.o)

# --------------------------------------------------------------------
#  Targets
# --------------------------------------------------------------------
.PHONY: all clean flash dump test

all: chronos-phantom.elf chronos-phantom.bin chronos-phantom.hex

chronos-phantom.elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lc -lm
	$(SIZE) $@

chronos-phantom.bin: chronos-phantom.elf
	$(OBJCOPY) -O binary $< $@

chronos-phantom.hex: chronos-phantom.elf
	$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

dump: chronos-phantom.elf
	$(OBJDUMP) -d -S $< > chronos-phantom.lst

flash: chronos-phantom.bin
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program chronos-phantom.bin 0x08000000 verify reset exit"

# --------------------------------------------------------------------
#  Host-side unit tests (no hardware required)
#  Compiles engine modules with a host GCC and runs assertions.
# --------------------------------------------------------------------
test:
	$(CC_HOST) -std=c11 -Wall -Wextra -DHOST_TEST -O0 -g \
		-I. -o test_ptp_engine \
		ptp_engine.c ntp_engine.c skew_gen.c covert_codec.c \
		eth_bridge.c ble_link.c tcxo_drvr.c imux_tamper.c \
		test_main.c -lm
	./test_ptp_engine
	@echo "All tests passed."

clean:
	rm -f $(OBJS) chronos-phantom.elf chronos-phantom.bin \
		chronos-phantom.hex chronos-phantom.map \
		chronos-phantom.lst test_ptp_engine

# Author: jayis1
# License: GPL-2.0