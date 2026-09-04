# MCTP Wraith firmware Makefile
# Author: jayis1
# Copyright (c) 2026 jayis1

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
TARGET = build/mctp_wraith_sim
SRCS = main.c \
       drivers/sideband_bus.c \
       drivers/policy_engine.c \
       drivers/pldm_codec.c \
       drivers/telemetry.c \
       drivers/radio_link.c
OBJS = $(SRCS:%.c=build/%.o)

all: $(TARGET)

build:
	mkdir -p build/drivers

build/%.o: %.c | build
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

.PHONY: all run clean
