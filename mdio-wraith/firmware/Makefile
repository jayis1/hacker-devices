# MDIO Wraith firmware Makefile
# Author: jayis1
# SPDX-License-Identifier: GPL-2.0-only

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=

PROJECT = mdio_wraith_sim
BUILD_DIR = build
SRC_DIR = .
DRV_DIR = $(SRC_DIR)/drivers

SRCS = $(SRC_DIR)/main.c \
       $(DRV_DIR)/mdio_bus.c \
       $(DRV_DIR)/profile.c \
       $(DRV_DIR)/telemetry.c \
       $(DRV_DIR)/radio.c \
       $(DRV_DIR)/safety.c

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
