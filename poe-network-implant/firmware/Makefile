# PhantomBridge PoE Network Implant — Firmware Makefile
# Cross-compile for ARM Cortex-M7 (STM32H743ZIT6)

# Toolchain
PREFIX  ?= arm-none-eabi-
CC       = $(PREFIX)gcc
AS       = $(PREFIX)gcc
LD       = $(PREFIX)ld
OBJCOPY  = $(PREFIX)objcopy
OBJDUMP  = $(PREFIX)objdump
SIZE     = $(PREFIX)size

# Target
TARGET   = phantombridge

# CPU flags
CPUFLAGS = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
OPTFLAGS = -O2 -flto

# Compiler flags
CFLAGS   = $(CPUFLAGS) $(OPTFLAGS) -Wall -Wextra -Wno-unused-parameter \
           -fno-common -ffunction-sections -fdata-sections \
           -fno-builtin-printf -fno-builtin-memset \
           -I. -Idrivers

# Assembler flags
ASFLAGS  = $(CPUFLAGS)

# Linker flags
LDFLAGS  = $(CPUFLAGS) -Wl,--gc-sections -Wl,--no-warn-rwx-segments \
           -T stm32h743.ld -nostartfiles -nostdlib \
           -Wl,--print-memory-usage

# Source files
CSRCS    = main.c \
           drivers/ksz9897.c \
           drivers/spi_flash.c

ASRCS    = startup.s

# Object files
COBJS    = $(CSRCS:.c=.o)
AOBJS    = $(ASRCS:.s=.o)
OBJS     = $(AOBJS) $(COBJS)

# Output
BUILDDIR = build

# Targets
.PHONY: all clean flash flash-uart debug

all: $(BUILDDIR)/$(TARGET).bin $(BUILDDIR)/$(TARGET).hex

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(TARGET).elf: $(OBJS) stm32h743.ld | $(BUILDDIR)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc

$(BUILDDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/$(TARGET).hex: $(BUILDDIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILDDIR)/$(TARGET).lst: $(BUILDDIR)/$(TARGET).elf
	$(OBJDUMP) -d -S $< > $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -c -o $@ $<

size: $(BUILDDIR)/$(TARGET).elf
	$(SIZE) $<

flash: $(BUILDDIR)/$(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
		-c "program $< verify reset exit 0x08000000"

flash-uart: $(BUILDDIR)/$(TARGET).bin
	stm32flash -w $< -v -g 0x08000000 /dev/ttyUSB0

debug: $(BUILDDIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32h7x.cfg &

clean:
	rm -rf $(BUILDDIR) *.o drivers/*.o