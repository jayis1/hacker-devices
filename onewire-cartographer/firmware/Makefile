# OneWire Cartographer firmware build
# Author: jayis1
# SPDX-License-Identifier: MIT
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -Ifirmware -Idrivers -DOWC_HOST_SIM=1
SOURCES := main.c drivers/onewire_phy.c drivers/analyzer.c drivers/policy.c
OBJECTS := $(SOURCES:.c=.o)
TARGET := onewire-cartographer

.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test: $(TARGET)
	./$(TARGET) --self-test
clean:
	rm -f $(OBJECTS) $(TARGET)
