# BLE Phantom Firmware — Makefile
# Cross-compilation for ARM Cortex-M4 (STM32F401)
#
# Toolchain: arm-none-eabi-gcc
# Target: ARM Cortex-M4F with hardware FPU

# ============================================================
# Toolchain
# ============================================================
CC       = arm-none-eabi-gcc
AS       = arm-none-eabi-as
LD       = arm-none-eabi-ld
OBJCOPY  = arm-none-eabi-objcopy
SIZE     = arm-none-eabi-size

# ============================================================
# Target configuration
# ============================================================
MCU      = cortex-m4
FPU      = fpv4-sp-d16
FLOAT_ABI = hard

# ============================================================
# Compiler flags
# ============================================================
CFLAGS   = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS  += -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter
CFLAGS  += -O2 -g3
CFLAGS  += -ffunction-sections -fdata-sections
CFLAGS  += -fno-common -fno-builtin -ffreestanding
CFLAGS  += -I.
CFLAGS  += -Idrivers
CFLAGS  += -DSTM32F401xC -DUSE_HAL_DRIVER=0 -DHSE_VALUE=8000000U

ASFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)

# ============================================================
# Linker flags
# ============================================================
LDFLAGS  = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
LDFLAGS += -nostartfiles -nostdlib
LDFLAGS += -T stm32f401.ld
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,--undefined=HardFault_Handler
LDFLAGS += -lgcc

# ============================================================
# Source files
# ============================================================
SRCDIR   = .
DRVDIR   = drivers

C_SRCS   = $(wildcard $(SRCDIR)/*.c)
C_SRCS  += $(wildcard $(DRVDIR)/*.c)

AS_SRCS  = $(wildcard $(SRCDIR)/*.s)

# ============================================================
# Object files
# ============================================================
OBJDIR   = build
C_OBJS   = $(patsubst %.c,$(OBJDIR)/%.o,$(C_SRCS))
AS_OBJS  = $(patsubst %.s,$(OBJDIR)/%.o,$(AS_SRCS))
OBJS     = $(C_OBJS) $(AS_OBJS)

# ============================================================
# Output files
# ============================================================
TARGET   = firmware
ELF      = $(OBJDIR)/$(TARGET).elf
HEX      = $(OBJDIR)/$(TARGET).hex
BIN      = $(OBJDIR)/$(TARGET).bin

# ============================================================
# Build targets
# ============================================================
.PHONY: all clean flash dfu size

all: $(HEX) $(BIN)

$(OBJDIR):
	mkdir -p $(OBJDIR)
	mkdir -p $(OBJDIR)/$(DRVDIR)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.s | $(OBJDIR)
	$(AS) $(ASFLAGS) -o $@ $<

$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

size: $(ELF)
	$(SIZE) $(ELF)

flash: $(HEX)
	openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg \
		-c "program $(HEX) verify reset exit"

dfu: $(BIN)
	dfu-util -a 0 -s 0x08000000:leave -D $(BIN)

clean:
	rm -rf $(OBJDIR)

# ============================================================
# Dependencies
# ============================================================
-include $(C_OBJS:.o=.d)

$(OBJDIR)/%.d: %.c | $(OBJDIR)
	@$(CC) $(CFLAGS) -MM -MT '$(OBJDIR)/$*.o' $< -o $@