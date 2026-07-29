# Makefile — NVMe-Phantom firmware (STM32H563, bare-metal)
#
# Author:  jayis1
# License: GPL-2.0
#
# Build:   make           # produces build/nvme_phantom.elf
# Flash:   make flash      # uses openocd + ST-Link
# Size:    make size
# Clean:   make clean

PROJECT   = nvme_phantom
BUILD_DIR = build

# ---- Toolchain ----
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
OPENOCD = openocd

# ---- MCU flags ----
MCU      = -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
CFLAGS  = $(MCU) -std=c11 -Wall -Wextra -Os -ffunction-sections -fdata-sections \
          -g3 -fno-common -fno-exceptions -I. -Idrivers
LDFLAGS = $(MCU) -T$(LINKER_SCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map \
          --specs=nano.specs --specs=nosys.specs

LINKER_SCRIPT = nvme_phantom.ld

# ---- Sources ----
SRCS = main.c \
       drivers/pcie_switch.c \
       drivers/nvme_parser.c \
       drivers/tlp_engine.c \
       drivers/fpga_spi.c \
       drivers/ble_c2.c \
       drivers/sd_pcap.c \
       drivers/usb_cdc.c \
       drivers/opal_probe.c \
       drivers/storage.c

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# ---- Targets ----
.PHONY: all flash size clean

all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(PROJECT).elf: $(OBJS) $(LINKER_SCRIPT)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD_DIR)/$(PROJECT).elf
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32h5x.cfg \
	  -c "program $(BUILD_DIR)/$(PROJECT).elf verify reset exit"

size: $(BUILD_DIR)/$(PROJECT).elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD_DIR)