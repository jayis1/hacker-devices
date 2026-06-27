# Tactile-Phantom — Touch-Controller Bus MITM Implant
# Makefile — Pico SDK build for RP2040 firmware
#
# Author: jayis1
# License: GPL-2.0
#
# Build requirements:
#   - Pico SDK (https://github.com/raspberrypi/pico-sdk)
#   - arm-none-eabi-gcc
#   - CMake 3.13+
#
# Build:
#   mkdir build && cd build
#   cmake .. -DPICO_SDK_PATH=/path/to/pico-sdk
#   make -j4
#
# Flash:
#   picotool load tactile_phantom.uf2 -a
#   (or hold BOOTSEL while plugging USB, copy UF2 to mass storage)

CMAKE_MINIMUM_VERSION = 3.13
PICO_SDK_VERSION = 1.5.1

# --- Project metadata ------------------------------------------------
PROJECT_NAME = tactile-phantom
PROJECT_VERSION = 1.0.0
PROJECT_AUTHOR = jayis1

# --- Source files ----------------------------------------------------
FIRMWARE_SRCS = \
    main.c \
    core1.c \
    protocol.c \
    touch_decode.c \
    injection.c \
    layout_infer.c \
    storage.c \
    ble_link.c

# PIO programs (assembled by pioasm)
PIO_SRCS = \
    bus_tap.pio

# Headers
FIRMWARE_HDRS = \
    board.h \
    registers.h

# --- CMake project definition ---------------------------------------
# This Makefile is a convenience wrapper around the CMake build.
# The actual build configuration is in CMakeLists.txt (below).

CMAKE_CONTENT = '\
cmake_minimum_required(VERSION 3.13)\n\
\n\
include(pico_sdk_import.cmake)\n\
\n\
project($(PROJECT_NAME) C CXX ASM)\n\
\n\
set(CMAKE_C_STANDARD 11)\n\
set(CMAKE_CXX_STANDARD 17)\n\
\n\
pico_sdk_init()\n\
\n\
add_executable($(PROJECT_NAME)\n\
    main.c\n\
    core1.c\n\
    protocol.c\n\
    touch_decode.c\n\
    injection.c\n\
    layout_infer.c\n\
    storage.c\n\
    ble_link.c\n\
)\n\
\n\
target_include_directories($(PROJECT_NAME) PRIVATE ${CMAKE_CURRENT_LIST_DIR})\n\
\n\
target_link_libraries($(PROJECT_NAME)\n\
    pico_stdlib\n\
    pico_multicore\n\
    pico_dma\n\
    pico_pio\n\
    pico_cyw43_arch_none\n\
    hardware_uart\n\
    hardware_spi\n\
    hardware_adc\n\
    hardware_gpio\n\
    hardware_clocks\n\
    pico_unique_id\n\
)\n\
\n\
# Generate PIO header from bus_tap.pio\n\
pico_generate_pio_header($(PROJECT_NAME) ${CMAKE_CURRENT_LIST_DIR}/bus_tap.pio)\n\
\n\
# USB CDC stdio\n\
pico_enable_stdio_usb($(PROJECT_NAME) 1)\n\
pico_enable_stdio_uart($(PROJECT_NAME) 0)\n\
\n\
# Build outputs: UF2, ELF, MAP\n\
pico_add_extra_outputs($(PROJECT_NAME))\n\
\n\
# Metadata\n\
target_compile_definitions($(PROJECT_NAME) PRIVATE\n\
    TP_AUTHOR=\"$(PROJECT_AUTHOR)\"\n\
    TP_VERSION=\"$(PROJECT_VERSION)\"\n\
)\n\
'

# --- Convenience targets --------------------------------------------
.PHONY: all clean cmake configure flash

all: cmake

# Generate CMakeLists.txt and configure build
cmake:
	@echo "Generating CMakeLists.txt..."
	@printf '$(CMAKE_CONTENT)' > CMakeLists.txt
	@echo "Configuring build..."
	@mkdir -p build
	@cd build && cmake .. -DPICO_SDK_PATH=$$(cat ../pico_sdk_path.txt 2>/dev/null || echo $$PICO_SDK_PATH) 2>&1 || true
	@echo "Building..."
	@cd build && make -j4 2>&1 || true
	@echo "Done. UF2 at: build/$(PROJECT_NAME).uf2"

configure:
	@printf '$(CMAKE_CONTENT)' > CMakeLists.txt
	@mkdir -p build
	@cd build && cmake .. -DPICO_SDK_PATH=$$(cat ../pico_sdk_path.txt 2>/dev/null || echo $$PICO_SDK_PATH)

# Flash via picotool
flash: all
	picotool load build/$(PROJECT_NAME).uf2 -a

# Clean
clean:
	rm -rf build CMakeLists.txt

# Syntax check (standalone, without Pico SDK)
check-syntax:
	@echo "Checking syntax (standalone)..."
	@arm-none-eabi-gcc -fsyntax-only -I. \
		-DPICO_STDIO_USB=1 -DPICO_STDIO_UART=0 \
		-include pico_stub.h \
		$(FIRMWARE_SRCS) 2>&1 || echo "Syntax check requires Pico SDK headers"

# Line count
lines:
	@echo "Firmware line counts:"
	@wc -l $(FIRMWARE_SRCS) $(FIRMWARE_HDRS) | tail -1
	@echo ""
	@echo "Per-file:"
	@wc -l $(FIRMWARE_SRCS) $(FIRMWARE_HDRS)