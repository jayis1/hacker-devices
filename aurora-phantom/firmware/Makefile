# Makefile — Aurora-Phantom firmware
# Author: jayis1   License: GPL-2.0
#
# Builds the ESP32-S3 application-core firmware and (optionally) synthesizes
# the iCE40UP5K FPGA signal-core image. The ESP32 build uses ESP-IDF; the
# FPGA build uses yosys + nextpnr-icepack. Both are optional: a host-test
# build (HOST=1) compiles the C sources against a mock HAL for unit tests
# without requiring any toolchain.

PROJECT  := aurora-phantom
AUTHOR   := jayis1

# ---- ESP32-S3 application core ----
IDF_PATH ?= $(HOME)/esp/esp-idf
IDF_TARGET := esp32s3

APP_SRCS := main.c \
            drivers/spad_driver.c \
            drivers/optics_driver.c \
            drivers/lockin.c \
            drivers/refresh_estimator.c \
            drivers/reconstruct.c \
            drivers/storage.c \
            drivers/ble_link.c \
            drivers/usb_cdc.c \
            drivers/power.c

APP_INCS := . drivers ../board.h ../registers.h

# ---- FPGA signal core ----
FPGA_PART  := iCE40UP5K
FPGA_PKG   := sg48
FPGA_PCF   := fpga/aurora.pcf
FPGA_SRC   := fpga/aurora_top.v fpga/spad_tdc.v fpga/lockin.v \
              fpga/refresh_fft.v fpga/framebuf.v fpga/spi_regfile.v
FPGA_OUT   := fpga/aurora.bin

# ---- Host test build ----
HOST_CC    ?= cc
HOST_CFLAGS := -Wall -Wextra -g -I. -Idrivers -DHOST_BUILD

.PHONY: all app fpga host-test clean flash monitor

all: app fpga

# ---- ESP-IDF build (requires idf.py in PATH) ----
app:
	@echo "[$(AUTHOR)] Building ESP32-S3 application core..."
	@idf.py build

flash:
	@idf.py -p /dev/ttyACM0 flash

monitor:
	@idf.py -p /dev/ttyACM0 monitor

# ---- FPGA synthesis ----
fpga: $(FPGA_OUT)

$(FPGA_OUT): $(FPGA_SRC) $(FPGA_PCF)
	@echo "[$(AUTHOR)] Synthesizing iCE40UP5K signal core..."
	yosys -p "synth_ice40 -top aurora_top -blif fpga/aurora.blif" $(FPGA_SRC)
	nextpnr-ice40 --$(FPGA_PART) --package $(FPGA_PKG) \
		--pcf $(FPGA_PCF) --blif fpga/aurora.blif --asc fpga/aurora.asc
	icepack fpga/aurora.asc $@

# ---- Host test build (no toolchain needed) ----
host-test:
	@echo "[$(AUTHOR)] Host test build..."
	$(HOST_CC) $(HOST_CFLAGS) -o host_test host_test_main.c main.c $(APP_SRCS) -lm
	./host_test

clean:
	-rm -f host_test fpga/aurora.blif fpga/aurora.asc $(FPGA_OUT)
	@if command -v idf.py >/dev/null 2>&1; then idf.py fullclean; fi