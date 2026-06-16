# Makefile for PCIe Screamer Firmware
# Target: RISC-V soft CPU (VexRiscv) on Lattice ECP5-45F
# Toolchain: riscv32-unknown-elf-gcc

CROSS_COMPILE ?= riscv32-unknown-elf-
CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE    = $(CROSS_COMPILE)size
GDB     = $(CROSS_COMPILE)gdb

# Architecture flags
ARCH_FLAGS = -march=rv32imc -mabi=ilp32

# Optimization and warning flags
OPT_FLAGS  = -Os -ffreestanding -fno-builtin -fno-stack-protector
WARN_FLAGS = -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

# Include paths
INC_FLAGS  = -I. -Idrivers

# CPU frequency for timing calculations
DEFINES    = -DF_CPU=125000000UL -DFPGA_ECP5

# Combined CFLAGS
CFLAGS  = $(ARCH_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(INC_FLAGS) $(DEFINES)
LDFLAGS = -T linker.ld -nostdlib -nostartfiles -Wl,--gc-sections

# Source files
SRCS = main.c \
       drivers/pcie.c \
       drivers/ddr3.c \
       drivers/usb_ft601.c \
       drivers/tlp_sniffer.c \
       drivers/tlp_injector.c \
       drivers/crc32c.c

# Object files
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

# Target
TARGET = build/firmware

# Build directories
BUILD_DIR = build
DRIVERS_BUILD_DIR = build/drivers

.PHONY: all clean flash debug disasm size

all: dirs $(TARGET).elf $(TARGET).bin $(TARGET).hex size

dirs:
	@mkdir -p $(BUILD_DIR) $(DRIVERS_BUILD_DIR)

$(TARGET).elf: $(addprefix $(BUILD_DIR)/,$(OBJS))
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "  ELF  $@"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<
	@echo "  CC   $<"

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo "  BIN  $@"

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@
	@echo "  HEX  $@"

disasm: $(TARGET).elf
	$(OBJDUMP) -d -S $< > $(TARGET).disasm
	@echo "  DIS  $(TARGET).disasm"

size: $(TARGET).elf
	@$(SIZE) $<

clean:
	rm -rf $(BUILD_DIR)

flash: $(TARGET).bin
	@echo "Flashing firmware to SPI flash at offset 0xC00000..."
	@echo "Use ecpprog or Lattice Diamond Programmer to write $(TARGET).bin"
	@echo "  ecpprog -o 0xC00000 $(TARGET).bin"
	@echo "  OR use companion app CMD_FW_UPDATE commands"

debug: $(TARGET).elf
	@echo "Starting GDB server..."
	$(GDB) -ex "target remote :3333" -ex "load" $(TARGET).elf

# Include dependency files
-include $(addprefix $(BUILD_DIR)/,$(DEPS))

# Help
help:
	@echo "PCIe Screamer Firmware Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build firmware (ELF, BIN, HEX)"
	@echo "  clean    - Remove build artifacts"
	@echo "  flash    - Flash firmware to SPI flash"
	@echo "  debug    - Start GDB debug session"
	@echo "  disasm   - Generate disassembly listing"
	@echo "  size     - Print section sizes"
	@echo ""
	@echo "Variables:"
	@echo "  CROSS_COMPILE - Toolchain prefix (default: riscv32-unknown-elf-)"
	@echo ""
	@echo "Output files:"
	@echo "  build/firmware.elf    - ELF executable"
	@echo "  build/firmware.bin    - Raw binary image"
	@echo "  build/firmware.hex    - Intel HEX format"
	@echo "  build/firmware.disasm - Disassembly listing"
