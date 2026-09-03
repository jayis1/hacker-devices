# SmartPack Phantom firmware Makefile
# Author: jayis1
# Copyright (c) 2026 jayis1

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
TARGET = build/smartpack_phantom_sim
SRCS = main.c \
       drivers/smbus_bus.c \
       drivers/battery_profile.c \
       drivers/auth_chip.c \
       drivers/telemetry.c \
       drivers/radio.c
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
