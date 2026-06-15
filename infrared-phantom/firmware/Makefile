# Infrared Phantom — Firmware Makefile
# Cross-compile for STM32H743 (Cortex-M7, hard float)

# Toolchain
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# MCU flags
CPU     = -mcpu=cortex-m7
FPU     = -mthumb -mfloat-abi=hard -mfpu=fpv5-d16
MCU     = $(CPU) $(FPU)

# Include paths
INCDIR  = -I. -Idrivers

# Compiler flags
CFLAGS  = -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter
CFLAGS += $(MCU) -std=gnu17 -O2 -g3
CFLAGS += -ffunction-sections -fdata-sections -fno-common
CFLAGS += -fno-builtin-printf -fno-builtin-malloc
CFLAGS += $(INCDIR)
CFLAGS += -DSTM32H743xx -DHSE_VALUE=25000000 -DUSE_HAL_DRIVER

# Linker flags
LDFLAGS = $(MCU) -T stm32h743vit6.ld -specs=nano.specs -specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=build/infrared-phantom.map

# Source files
SRCS    = main.c
DRIVERS = drivers/ir_receiver.c drivers/ir_transmitter.c drivers/w25q128.c drivers/ssd1306.c

OBJS    = $(SRCS:.c=.o) $(DRIVERS:.c=.o)

# Output
BUILDDIR = build
TARGET  = $(BUILDDIR)/infrared-phantom

.PHONY: all clean flash dfu size

all: $(TARGET).bin $(TARGET).hex size

$(TARGET).elf: $(OBJS) stm32h743vit6.ld | $(BUILDDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lm -lc -lnosys

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).lst: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

size: $(TARGET).elf
	$(SIZE) $<

clean:
	rm -rf $(BUILDDIR) *.o drivers/*.o

flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit"

dfu: $(TARGET).bin
	dfu-util -a 0 -s 0x08000000:leave -D $<