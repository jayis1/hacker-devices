# DALI Sentinel firmware build
# Author: jayis1
CC ?= gcc
BUILD := build
TARGET := $(BUILD)/dali-sentinel
SRC := main.c drivers/dali_phy.c drivers/analyzer.c drivers/storage.c
OBJ := $(SRC:%.c=$(BUILD)/%.o)
CPPFLAGS := -I. -Idrivers
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror
LDFLAGS :=
.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
test: $(TARGET)
	./$(TARGET) --self-test
clean:
	rm -rf $(BUILD)
