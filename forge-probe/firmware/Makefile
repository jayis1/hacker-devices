# Makefile — Forge-Probe Firmware Build System
# Author: jayis1
# License: Proprietary — Authorized Security Research Use Only
#
# Targets:
#   make all          — Build firmware ELF + HEX + BIN
#   make flash        — Flash via ST-Link (SWD)
#   make debug        — Build with debug symbols and semihosting
#   make clean        — Remove build artifacts
#   make docs         — Generate Doxygen documentation
#   make size         — Show section sizes

# ─── Toolchain ───────────────────────────────────────────────────────────────
CROSS_COMPILE ?= arm-none-eabi-
CC       := $(CROSS_COMPILE)gcc
LD       := $(CROSS_COMPILE)gcc
OBJCOPY  := $(CROSS_COMPILE)objcopy
OBJDUMP  := $(CROSS_COMPILE)objdump
SIZE     := $(CROSS_COMPILE)size
GDB      := $(CROSS_COMPILE)gdb
OPENOCD  ?= openocd

# ─── Project ─────────────────────────────────────────────────────────────────
TARGET   := forge-probe
BOARD    := STM32H743IIK6

# ─── Directories ─────────────────────────────────────────────────────────────
BUILD_DIR  := build
SRC_DIR    := .
INC_DIR    := .
DRV_DIR    := drivers
OBJ_DIR    := $(BUILD_DIR)/obj

# ─── Source Files ────────────────────────────────────────────────────────────
SRCS := \
	main.c \
	drivers/gpio.c \
	drivers/timing.c \
	drivers/usb_dev.c \
	drivers/sdmmc.c \
	drivers/mpio.c \
	drivers/fpga_spi.c

OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# ─── MCU-Specific Flags (STM32H743, Cortex-M7) ────────────────────────────────
CPU       := -mcpu=cortex-m7
FPU       := -mfpu=fpv5-d16 -mfloat-abi=hard
MCU_FLAGS := $(CPU) -mthumb $(FPU) -mthumb-interwork

# ─── Linker ───────────────────────────────────────────────────────────────────
LD_SCRIPT := stm32h743ii_flash.ld
LDFLAGS   := $(MCU_FLAGS) -T$(LD_SCRIPT) \
             -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
             -Wl,--cref -Wl,--print-memory-usage \
             -specs=nano.specs -specs=nosys.specs \
             -Wl,--start-group -lc -lm -lstdc++ -lsupc++ -Wl,--end-group

# ─── Compiler Flags ───────────────────────────────────────────────────────────
CFLAGS := $(MCU_FLAGS) \
          -O2 \
          -g3 \
          -Wall -Wextra -Wpedantic \
          -Wshadow -Wdouble-promotion -Wformat=2 \
          -Wmissing-prototypes -Wstrict-prototypes \
          -Wconversion -Wsign-conversion \
          -Wunused-parameter -Wunused-function \
          -ffunction-sections -fdata-sections \
          -fstack-usage -fomit-frame-pointer \
          -fsingle-precision-constant \
          -flto \
          -DSTM32H743xx -DUSE_HAL_DRIVER \
          -I$(INC_DIR) -I$(DRV_DIR)

# Release build (no asserts)
ifneq ($(DEBUG),1)
CFLAGS += -DNDEBUG -Os
else
CFLAGS += -DDEBUG -Og -ggdb3
endif

# ─── Rules ────────────────────────────────────────────────────────────────────

.PHONY: all flash debug clean docs size dist

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Object compilation
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
	@echo "  CC    $<"

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
	@echo "  LD    $@"
	$(SIZE) $@

# Post-processing
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@
	@echo "  HEX   $@"

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	@echo "  BIN   $@"

# Flash via OpenOCD + ST-Link
flash: $(BUILD_DIR)/$(TARGET).elf
	$(OPENOCD) -f interface/stlink-v3.cfg \
	           -f target/stm32h7x.cfg \
	           -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# Debug build
debug: CFLAGS += -DDEBUG -Og -ggdb3 -fsanitize=undefined
debug: all

# GDB session
gdb: $(BUILD_DIR)/$(TARGET).elf
	$(GDB) -ex "target extended-remote localhost:3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       $<

# Clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "  CLEAN"

# Size report
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<
	@echo ""
	@echo "Stack analysis (from .su files):"
	-cat $(OBJ_DIR)/*.su | sort -t: -k2 -rn | head -20

# Doxygen docs
docs:
	@mkdir -p docs
	doxygen Doxyfile 2>/dev/null || echo "Install doxygen for docs generation"

# Disassembly
disasm: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -S -d $< > $(BUILD_DIR)/$(TARGET).dis

# Dependency files
-include $(DEPS)