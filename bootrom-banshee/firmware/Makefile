# BootROM Banshee firmware Makefile
# Author: jayis1
# Copyright (c) 2026 jayis1

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
TARGET = build/bootrom_banshee_sim
SRCS = main.c \
       drivers/spi_link.c \
       drivers/overlay_store.c \
       drivers/glitch_engine.c \
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
