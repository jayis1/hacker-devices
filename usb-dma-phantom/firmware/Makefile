# USB DMA Phantom — Firmware Makefile
# Cross-compile for ARM Cortex-M4F (STM32F423)

# Toolchain
PREFIX  ?= arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
LD      = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# MCU parameters
MCU     = cortex-m4
DEFS    = -DSTM32F423xx -DHSE_VALUE=25000000UL -DARM_MATH_CM4
CPUFLAGS= -mcpu=$(MCU) -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS  = -Wall -Wextra -Wno-unused-parameter -O2 -g3 $(CPUFLAGS) $(DEFS) \
          -ffunction-sections -fdata-sections -fno-common -fno-builtin \
          -ffreestanding -nostdlib -I. -Idrivers
ASFLAGS = $(CPUFLAGS) -D__ASSEMBLY__
LDFLAGS = -Tstm32f423ch.ld -nostartfiles -Wl,--gc-sections \
          -Wl,-Map=$(@:.elf=.map) -Wl,--print-memory-usage

# Source files
SRCS    = main.c \
          startup.c \
          drivers/spi4.c \
          drivers/w25q128.c \
          drivers/xio2001.c

# Object files
OBJS    = $(SRCS:.c=.o) startup.o

# Targets
TARGET  = usb-dma-phantom

.PHONY: all clean flash dfu-flash size objdump

all: $(TARGET).elf $(TARGET).hex $(TARGET).bin

$(TARGET).elf: $(OBJS) stm32f423ch.ld
	$(CC) $(CPUFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lgcc

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

size: $(TARGET).elf
	$(SIZE) $<

objdump: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $(TARGET).lst

flash: $(TARGET).hex
	openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg \
		-c "program $< verify reset exit"

dfu-flash: $(TARGET).bin
	dfu-util -a 0 -s 0x08000000:leave -D $<

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex $(TARGET).bin \
	      $(TARGET).map $(TARGET).lst