# PDM Trust Probe firmware build
# Author: jayis1
# SPDX-License-Identifier: MIT
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS += -I. -Idrivers -DPTP_HOST_SIM=1
SOURCES := main.c drivers/pdm_capture.c drivers/analyzer.c drivers/diagnostics.c drivers/protocol.c
OBJECTS := $(SOURCES:.c=.o)
TARGET := pdm-trust-probe
.PHONY: all test clean
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test: $(TARGET)
	./$(TARGET) --self-test
clean:
	rm -f $(OBJECTS) $(TARGET)
