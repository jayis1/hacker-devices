# Substation Firmware Makefile
# Cross-compile for ARM Cortex-M4F (CC1352P)

# Toolchain
PREFIX  ?= arm-none-eabi-
CC       = $(PREFIX)gcc
AS       = $(PREFIX)gcc
LD       = $(PREFIX)ld
OBJCOPY  = $(PREFIX)objcopy
OBJDUMP  = $(PREFIX)objdump
SIZE     = $(PREFIX)size

# Target MCU
MCU      = cortex-m4f
CPUFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# SDK paths
SDK_PATH ?= /opt/ti/simplelink_sdk

# Compiler flags
CFLAGS  = $(CPUFLAGS) -Wall -Wextra -Werror -std=gnu11 -O2 -g3
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += -fno-builtin -fno-strict-aliasing
CFLAGS += -I. -Iinclude -Idrivers
CFLAGS += -I$(SDK_PATH)/source
CFLAGS += -DCC1352P -D__CORTEX_M4F -DBOARD_SUBSTATION

# Linker flags
LDFLAGS = $(CPUFLAGS) -nostartfiles -nostdlib
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -T linkers/cc1352p.ld

# Source files
SRCS    = main.c
SRCS   += board.c
SRCS   += startup.c
SRCS   += drivers/radio_subghz.c
SRCS   += drivers/sdram.c
SRCS   += drivers/ble_comms.c
SRCS   += drivers/spi_flash.c
SRCS   += drivers/atecc608a.c
SRCS   += drivers/sd_card.c
SRCS   += drivers/ws2812b.c
SRCS   += drivers/usb_cdc.c

# Object files
OBJS    = $(SRCS:.c=.o)

# Targets
TARGET  = substation

.PHONY: all clean flash debug size

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin size

$(TARGET).elf: $(OBJS) linkers/cc1352p.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(SDK_PATH)/source/ti/devices/cc13x2_cc26x2/driverlib/lib/driverlib.a -lm

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(CPUFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin $(TARGET).map

flash: $(TARGET).elf
	openocd -f interface/cmsis-dap.cfg -f target/cc1352p.cfg \
		-c "program $(TARGET).elf verify reset exit"

debug: $(TARGET).elf
	openocd -f interface/cmsis-dap.cfg -f target/cc1352p.cfg &
	$(PREFIX)gdb $(TARGET).elf -ex "target remote :3333"

size: $(TARGET).elf
	$(SIZE) -A $(TARGET).elf