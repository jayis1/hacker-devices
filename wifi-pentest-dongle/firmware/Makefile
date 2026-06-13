# WFP-100 Firmware Makefile
# Cross-compile for StarFive JH7110 (RISC-V 64-bit)

CROSS_COMPILE ?= riscv64-unknown-elf-
CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE    = $(CROSS_COMPILE)size

# Compiler flags
CFLAGS  = -march=rv64imac -mabi=lp64 -mcmodel=medany -fno-builtin -ffreestanding
CFLAGS += -O2 -Wall -Wextra -Werror
CFLAGS += -I. -Idrivers

# Linker flags
LDFLAGS = -nostdlib -T wfp100.lds

# Source files
SRCS_SPL = main.c drivers/axp2101.c
SRCS_DRV = drivers/ax210_pcie.c drivers/usb_gadget.c

# Object files
OBJS_SPL = $(SRCS_SPL:.c=.o)
OBJS_DRV = $(SRCS_DRV:.c=.o)

# Targets
.PHONY: all clean spl drivers

all: spl drivers

spl: u-boot-spl.bin
drivers: ax210_pcie.ko usb_gadget.ko

# SPL build
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

u-boot-spl.elf: $(OBJS_SPL)
	$(CC) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

u-boot-spl.bin: u-boot-spl.elf
	$(OBJCOPY) -O binary $< $@

# Kernel module build (requires Linux source tree)
KDIR ?= /lib/modules/$(shell uname -r)/build

ax210_pcie.ko: drivers/ax210_pcie.c drivers/ax210_pcie.h
	$(MAKE) -C $(KDIR) M=$(shell pwd)/drivers modules

usb_gadget.ko: drivers/usb_gadget.c drivers/usb_gadget.h
	$(MAKE) -C $(KDIR) M=$(shell pwd)/drivers modules

clean:
	rm -f *.o *.elf *.bin *.ko drivers/*.o
	rm -f drivers/.*.cmd drivers/*.ko drivers/*.mod.c drivers/Module.symvers