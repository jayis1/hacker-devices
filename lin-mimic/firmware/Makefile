# LIN Mimic firmware build
# Author: jayis1
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -DHOST_SIM
SRC := main.c board.c drivers/lin.c drivers/policy.c drivers/protocol.c
OBJ := $(SRC:.c=.o)
TARGET := lin-mimic

.PHONY: all clean test size
all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
test: clean $(TARGET)
	./$(TARGET)
size: $(TARGET)
	size $(TARGET)
clean:
	rm -f $(OBJ) $(TARGET)
# Production example: make CC=arm-none-eabi-gcc CFLAGS='-mcpu=cortex-m4 -mthumb -Os ...'
