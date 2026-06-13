# RF Transceiver Tool — Firmware Makefile
#
# Build system for STM32F401CCU6 firmware using arm-none-eabi-gcc
# Targets:
#   all       - Build firmware.elf and firmware.bin
#   flash     - Flash firmware.bin via STM32CubeProgrammer or st-flash
#   clean     - Remove build artifacts
#   size      - Show firmware size breakdown

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc
LD      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

# MCU settings
MCU      = cortex-m4
FPU      = fpv4-sp-d16
CPUFLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=hard

# Include paths
INCDIR = -I. -Idrivers

# Compiler flags
CFLAGS  = $(CPUFLAGS) -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration
CFLAGS += -ffunction-sections -fdata-sections -fno-common -fno-builtin
CFLAGS += -Os -g3 $(INCDIR)
CFLAGS += -DSTM32F401xC -DUSE_FULL_ASSERT -DARM_MATH_CM4

# Linker flags
LDFLAGS = $(CPUFLAGS) -T STM32F401CCUX_FLASH.ld -Wl,--gc-sections
LDFLAGS += -Wl,-Map=firmware.map -Wl,--print-memory-usage
LDFLAGS += --specs=nano.specs --specs=nosys.specs

# Source files
SRCS    = main.c
DRVSRC  = drivers/cc1101.c drivers/nrf24l01.c drivers/ssd1306.c drivers/spi_dma.c
OBJS    = $(SRCS:.c=.o) $(DRVSRC:.c=.o)

# Output
TARGET  = firmware
BINARY  = $(TARGET).bin

# Default target
.PHONY: all clean flash size

all: $(TARGET).elf $(BINARY)

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ -lm -lc_nano -lgcc

$(BINARY): $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(CPUFLAGS) -c -o $@ $<

flash: $(BINARY)
	@echo "Flashing via ST-Link..."
	st-flash write $(BINARY) 0x08004000

flash-dfu: $(TARGET).elf
	@echo "Flashing via DFU..."
	stm32flash -w $(BINARY) /dev/ttyUSB0

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

clean:
	rm -f $(OBJS) $(TARGET).elf $(BINARY) $(TARGET).map

# Dependencies (simplified)
main.o: main.c board.h registers.h drivers/cc1101.h drivers/nrf24l01.h drivers/ssd1306.h drivers/spi_dma.h
drivers/cc1101.o: drivers/cc1101.c drivers/cc1101.h board.h registers.h
drivers/nrf24l01.o: drivers/nrf24l01.c drivers/nrf24l01.h board.h registers.h
drivers/ssd1306.o: drivers/ssd1306.c drivers/ssd1306.h board.h registers.h
drivers/spi_dma.o: drivers/spi_dma.c drivers/spi_dma.h board.h registers.h