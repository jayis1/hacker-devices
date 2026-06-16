#===============================================================================
# Spectre-Sniffer Firmware Makefile
# Author: jayis1
# Description: Build system for ESP32-S3 firmware with FPGA bitstream
#===============================================================================

# Project name
PROJECT_NAME := spectre-sniffer

# Toolchain
CROSS := xtensa-esp32s3-elf-
CC    := $(CROSS)gcc
CXX   := $(CROSS)g++
LD    := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE  := $(CROSS)size

# Directories
SRC_DIR    := .
BUILD_DIR  := build
DRV_DIR    := drivers
SIG_DIR    := signal
UI_DIR     := ui
NET_DIR    := net
UTIL_DIR   := utils

# Source files
SRCS := \
	main.c \
	$(DRV_DIR)/adc.c \
	$(DRV_DIR)/fpga.c \
	$(DRV_DIR)/display.c \
	$(DRV_DIR)/sdcard.c \
	$(DRV_DIR)/rf_frontend.c \
	$(DRV_DIR)/rtc.c \
	$(SIG_DIR)/fft_processing.c \
	$(SIG_DIR)/tempest_decode.c \
	$(SIG_DIR)/dpa_engine.c \
	$(SIG_DIR)/cpa_engine.c \
	$(SIG_DIR)/keystroke_analyzer.c \
	$(UI_DIR)/main_screen.c \
	$(UI_DIR)/tempest_view.c \
	$(UI_DIR)/crypto_analyzer.c \
	$(UI_DIR)/menu.c \
	$(NET_DIR)/wifi_exfil.c \
	$(NET_DIR)/ble_exfil.c \
	$(UTIL_DIR)/circular_buffer.c \
	$(UTIL_DIR)/config_manager.c

# Object files
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

# Include paths
INCLUDES := \
	-I. \
	-I$(DRV_DIR) \
	-I$(SIG_DIR) \
	-I$(UI_DIR) \
	-I$(NET_DIR) \
	-I$(UTIL_DIR) \
	-I$(IDF_PATH)/components/esp_common/include \
	-I$(IDF_PATH)/components/esp_system/include \
	-I$(IDF_PATH)/components/freertos/include \
	-I$(IDF_PATH)/components/driver/include \
	-I$(IDF_PATH)/components/spi_flash/include \
	-I$(IDF_PATH)/components/soc/include \
	-I$(IDF_PATH)/components/esp_wifi/include \
	-I$(IDF_PATH)/components/bt/include \
	-I$(IDF_PATH)/components/fatfs/src

# Compiler flags
CFLAGS := \
	-g \
	-O2 \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter \
	-ffunction-sections \
	-fdata-sections \
	-mlongcalls \
	-march=xtensavx \
	-DCONFIG_IDF_TARGET_ESP32S3 \
	-DESP_PLATFORM \
	-DPROJECT_NAME=\"$(PROJECT_NAME)\" \
	-DPROJECT_VERSION=\"1.0.0\" \
	-DAUTHOR=\"jayis1\" \
	$(INCLUDES)

# Linker flags
LDFLAGS := \
	-Wl,--gc-sections \
	-Wl,--start-group \
	-lm \
	-lc \
	-lgcc \
	-lhal \
	-lfreertos \
	-ldriver \
	-lspi_flash \
	-lfatfs \
	-lwpa_supplicant \
	-lesp_wifi \
	-lbt \
	-Wl,--end-group \
	-T esp32s3.ld

# FPGA bitstream (pre-compiled, included as binary blob)
FPGA_BITSTREAM := fpga/bitstream.bin
FPGA_OBJ := $(BUILD_DIR)/fpga_bitstream.o

#===============================================================================
# Targets
#===============================================================================

.PHONY: all clean flash monitor size

all: $(BUILD_DIR)/$(PROJECT_NAME).bin

# Link final ELF
$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJS) $(FPGA_OBJ)
	@mkdir -p $(@D)
	$(LD) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(SIZE) $@

# Generate binary for flashing
$(BUILD_DIR)/$(PROJECT_NAME).bin: $(BUILD_DIR)/$(PROJECT_NAME).elf
	$(OBJCOPY) -O binary $< $@
	@echo "=== Build complete: $@ ==="
	@echo "Flash with: esptool.py --chip esp32s3 write_flash 0x0 $@"

# Compile C sources
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile FPGA bitstream into object file
$(FPGA_OBJ): $(FPGA_BITSTREAM)
	@mkdir -p $(@D)
	$(OBJCOPY) -I binary -O elf32-xtensa-le -B xtensavx \
		--rename-section .data=.rodata,alloc,load,readonly,data \
		$< $@

# Flash to device
flash: $(BUILD_DIR)/$(PROJECT_NAME).bin
	esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
		write_flash --flash_mode dio --flash_size 16MB 0x0 $<

# Monitor serial output
monitor:
	picocom -b 115200 /dev/ttyUSB0

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Show code statistics
stats:
	@echo "=== Code Statistics ==="
	@wc -l $(SRCS) $(wildcard *.h) $(wildcard $(DRV_DIR)/*.h) \
		$(wildcard $(SIG_DIR)/*.h) $(wildcard $(UI_DIR)/*.h) \
		$(wildcard $(NET_DIR)/*.h) $(wildcard $(UTIL_DIR)/*.h)
	@echo "---"
	@echo "Total C source files: $(words $(SRCS))"
	@echo "Total object files:   $(words $(OBJS))"
