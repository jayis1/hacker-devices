# Makefile — Harmonic Reaper Firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build:  make
# Flash:  make flash
# Clean:  make clean
# Size:   make size
#
# Toolchain: arm-none-eabi-gcc (GNU Arm Embedded 10-2021-q4-major or later)

# ========================================================================
#  Toolchain
# ========================================================================

CC       = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size

# ========================================================================
#  Target MCU — nRF52840 QFAA (Cortex-M4F, 64 MHz, 1 MB flash, 256 KB RAM)
# ========================================================================

MCU      = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
DEFS     = -DNRF52840_XXAA -DBOARD_CUSTOM
INCLUDES = -I. -Idrivers

# ========================================================================
#  Build flags
# ========================================================================

CFLAGS   = $(MCU) $(DEFS) $(INCLUDES) -Os -g3 -Wall -Wextra -std=c11 \
           -ffunction-sections -fdata-sections -fno-builtin -fshort-enums
LDFLAGS  = $(MCU) -T linker.ld -Wl,--gc-sections -Wl,-Map=build/reaper.map \
           --specs=nano.specs --specs=nosys.specs
LIBS     = -lc -lnosys

# ========================================================================
#  SoftDevice (Nordic S140 v7.3.0) — linked first, app at 0x27000
# ========================================================================

SOFTDEVICE    = s140_nrf52_7.3.0_softdevice.hex
APP_OFFSET    = 0x27000

# ========================================================================
#  Sources
# ========================================================================

SRCS = \
    main.c \
    drivers/adf4159.c \
    drivers/qpl9547.c \
    drivers/ad9361.c \
    drivers/ad9226_if.c \
    drivers/fpga_spi.c \
    drivers/oled_ssd1306.c \
    drivers/imu_icm42688.c \
    drivers/ble_c2.c \
    drivers/power_mgmt.c \
    drivers/storage.c \
    drivers/usb_cdc.c

OBJS = $(patsubst %.c, build/%.o, $(SRCS))

# ========================================================================
#  Targets
# ========================================================================

.PHONY: all flash clean size disasm

all: build/reaper.elf build/reaper.hex

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/reaper.elf: $(OBJS) linker.ld
	@mkdir -p build
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	$(SIZE) $@

build/reaper.hex: build/reaper.elf
	$(OBJCOPY) -O ihex $< $@

flash: build/reaper.hex
	nrfjprog --program $(SOFTDEVICE) --sectorerase --reset
	nrfjprog --program build/reaper.hex --sectorerase --reset

size: build/reaper.elf
	$(SIZE) build/reaper.elf

disasm: build/reaper.elf
	$(OBJDUMP) -d build/reaper.elf > build/reaper.dis

clean:
	rm -rf build

# ========================================================================
#  Header dependencies
# ========================================================================

build/main.o: board.h registers.h \
              drivers/adf4159.h drivers/qpl9547.h drivers/ad9361.h \
              drivers/ad9226_if.h drivers/fpga_spi.h drivers/oled_ssd1306.h \
              drivers/imu_icm42688.h drivers/ble_c2.h drivers/power_mgmt.h \
              drivers/storage.h drivers/usb_cdc.h

build/drivers/ble_c2.o: drivers/storage.h

# EOF — Makefile — jayis1