# Type-C Specter firmware Makefile
# Author: jayis1
# Copyright (c) 2026 jayis1

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
TARGET = build/typec_specter_sim
SRCS = main.c \
       drivers/pd_phy.c \
       drivers/cc_mux.c \
       drivers/vbus_meter.c \
       drivers/script_engine.c \
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
