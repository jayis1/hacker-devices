# NFC Relay Phantom — Firmware Build System

TOOLCHAIN = arm-none-eabi-
CC = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE = $(TOOLCHAIN)size
OBJDUMP = $(TOOLCHAIN)objobjdump

# Target MCU
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# Defines
DEFS = -DSTM32L4S5xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 \
       -DSYSTEM_CLOCK=120000000 -DARM_MATH_CM4

# Include paths
INCS = -I. -Idrivers -I$(STM32_CUBE)/Drivers/CMSIS/Device/ST/STM32L4xx/Include \
       -I$(STM32_CUBE)/Drivers/CMSIS/Include \
       -I$(STM32_CUBE)/Drivers/STM32L4xx_HAL_Driver/Inc

# Compiler flags
CFLAGS = $(MCU) $(DEFS) $(INCS) -O2 -Wall -Wextra -Wno-unused-parameter \
         -fdata-sections -ffunction-sections -std=gnu11 \
         -fno-common -fno-builtin -fomit-frame-pointer

# Linker flags
LDFLAGS = $(MCU) -T STM32L4S5VITx_FLASH.ld -Wl,--gc-sections \
          -Wl,--defsym=__heap_size=0x200 -Wl,--defsym=__stack_size=0x400 \
          -Wl,-Map=build/nfc_phantom.map -Wl,--print-memory-usage \
          -specs=nano.specs -specs=nosys.specs

# Source files
SRCS = main.c \
       drivers/spi_dma.c \
       drivers/pn5180.c \
       drivers/em4095.c \
       system_stm32l4xx.c \
       startup_stm32l4s5xx.s

OBJS = $(patsubst %.c,build/%.o,$(patsubst %.s,build/%.o,$(SRCS)))

# Output
TARGET = build/nfc_phantom
BIN = $(TARGET).bin
HEX = $(TARGET).hex
ELF = $(TARGET).elf

.PHONY: all clean flash flash-dfu size

all: $(BIN) $(HEX)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

size: $(ELF)
	$(SIZE) -A $(ELF)

flash: $(HEX)
	openocd -f interface/cmsis-dap.cfg -f target/stm32l4s5.cfg \
		-c "program $(HEX) verify reset exit"

flash-dfu: $(BIN)
	dfu-util -a 0 -s 0x08000000:leave -D $(BIN)

clean:
	rm -rf build/
	mkdir -p build