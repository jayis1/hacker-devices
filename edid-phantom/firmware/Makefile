# EDID Phantom firmware Makefile
# Author: jayis1
# Copyright (c) 2026 jayis1

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
TARGET = build/edid_phantom_sim
SRCS = main.c \
       drivers/log.c \
       drivers/ddc.c \
       drivers/cec.c \
       drivers/profile.c \
       drivers/radio.c \
       drivers/safety.c
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
