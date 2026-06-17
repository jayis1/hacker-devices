#
# Makefile — HDMI-Siphon ESP32-S3 firmware build
# Author: jayis1
# Version: 1.0.0
#
# Target: ESP32-S3 (ESP32-S3-WROOM-1 module)
# Toolchain: ESP-IDF v5.1+
#
# Usage:
#   make menuconfig    - Configure project settings
#   make build         - Build firmware
#   make flash         - Flash to device via USB
#   make monitor       - Open serial monitor
#   make clean         - Clean build artifacts
#   make fullflash     - Build, flash, and monitor
#

# =========================================================================
# Project Configuration
# =========================================================================

PROJECT_NAME := hdmi-siphon
TARGET       := esp32s3
BOARD        := hdmi-siphon-v1

# Source directories
SRC_DIRS     := . drivers

# Source files (auto-discovered from SRC_DIRS)
SRC_FILES    := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))

# Include directories
INC_DIRS     := . drivers

# =========================================================================
# Compiler and Linker Flags
# =========================================================================

CFLAGS := \
    -std=gnu11 \
    -Wall \
    -Wextra \
    -Werror=implicit-function-declaration \
    -Werror=return-type \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -O2 \
    -g \
    -ffunction-sections \
    -fdata-sections \
    -mlongcalls \
    -DBOARD_$(BOARD) \
    -DCONFIG_CPU_FREQ_MHZ=240 \
    -DCONFIG_APB_FREQ_MHZ=80

LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -u app_main

# =========================================================================
# Build Targets
# =========================================================================

.PHONY: all menuconfig build flash monitor clean fullflash idf

# Default: build
all: build

# Launch ESP-IDF menuconfig
menuconfig:
	idf.py menuconfig

# Build using ESP-IDF build system
build:
	idf.py set-target $(TARGET)
	idf.py build

# Flash firmware to device
flash:
	idf.py -p $(PORT) flash

# Open serial monitor
monitor:
	idf.py -p $(PORT) monitor

# Clean build artifacts
clean:
	idf.py fullclean
	rm -f sdkconfig.old

# Build, flash, and monitor in sequence
fullflash: build flash monitor

# Pass through to idf.py
idf:
	idf.py $(CMD)

# =========================================================================
# Default Serial Port (override with PORT=/dev/ttyUSB1)
# =========================================================================

PORT ?= /dev/ttyUSB0

# =========================================================================
# Dependencies
# =========================================================================

# Generate dependency files automatically
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $(addprefix -I,$(INC_DIRS)) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SRC_FILES:.c=.d)

# =========================================================================
# Help
# =========================================================================

help:
	@echo "HDMI-Siphon Firmware Build System"
	@echo "=================================="
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build firmware (default)"
	@echo "  menuconfig   - Configure ESP-IDF settings"
	@echo "  build        - Build firmware binary"
	@echo "  flash        - Flash to device (PORT=/dev/ttyUSB0)"
	@echo "  monitor      - Open serial monitor"
	@echo "  clean        - Remove build artifacts"
	@echo "  fullflash    - Build + flash + monitor"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Author: jayis1"
	@echo "License: Proprietary — Authorized Security Research Use Only"
