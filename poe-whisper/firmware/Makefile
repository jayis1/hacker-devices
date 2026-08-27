# PoE Whisper firmware Makefile
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0-only

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?= -lm

PROJECT = poe_whisper_sim
BUILD_DIR = build
SRC_DIR = .
DRV_DIR = $(SRC_DIR)/drivers

SRCS = $(SRC_DIR)/main.c        $(DRV_DIR)/poe_port.c        $(DRV_DIR)/lldp.c        $(DRV_DIR)/signature.c        $(DRV_DIR)/relay.c        $(DRV_DIR)/radio.c

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

all: $(BUILD_DIR)/$(PROJECT)

$(BUILD_DIR)/$(PROJECT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BUILD_DIR)/$(PROJECT)
	./$(BUILD_DIR)/$(PROJECT)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
