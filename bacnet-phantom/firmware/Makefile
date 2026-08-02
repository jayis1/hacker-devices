#
# Makefile — BACnet Phantom firmware wrapper around idf.py
#
# Author: jayis1
# License: GPLv3
#
# The firmware is ESP-IDF v5.1+ based. This Makefile exists so the build
# command is identical across devices in this repo and so CI can invoke a
# single target. Run `make` in this directory to build.
#
.PHONY: all set-target build flash monitor clean fuzzer-test

IDF_PATH ?= $(HOME)/esp/esp-idf
IDF_PY   := $(IDF_PATH)/tools/idf.py
TARGET   := esp32s3

all: set-target build

set-target:
	$(IDF_PY) set-target $(TARGET)

build:
	$(IDF_PY) build

flash: build
	$(IDF_PY) -p $(PORT) flash monitor

monitor:
	$(IDF_PY) -p $(PORT) monitor

clean:
	$(IDF_PY) fullclean

# Sanity check: ensure the author macro is present in every source file.
fuzzer-test:
	@echo "[*] BACnet Phantom author / stub audit (jayis1)"
	@grep -L "jayis1" *.c *.h 2>/dev/null && \
	    echo "[!] Files missing author tag above" && exit 1 || \
	    echo "[+] All source files credit jayis1"