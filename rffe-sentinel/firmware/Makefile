# RFFE Sentinel host-validation firmware build
# Author: jayis1
# SPDX-License-Identifier: MIT
CC ?= cc
CPPFLAGS += -I. -Idrivers -DRS_HOST_SIM=1
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
SOURCES := main.c drivers/rffe.c drivers/policy.c drivers/protocol.c
OBJECTS := $(SOURCES:.c=.o)
TARGET := rffe-sentinel

.PHONY: all clean test check
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET) --self-test

check: test
	./$(TARGET) --demo > /dev/null

clean:
	rm -f $(OBJECTS) $(TARGET)
