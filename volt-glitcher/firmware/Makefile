CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
CFLAGS = -mcpu=cortex-m7 -mthumb -O2 -Wall -I. -I./drivers
LDFLAGS = -specs=nosys.specs -T stm32h723.ld

SRCS = main.c drivers/fpga.c drivers/glitch.c
OBJS = $(SRCS:.c=.o)

all: volt-glitcher.bin

volt-glitcher.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

volt-glitcher.bin: volt-glitcher.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o drivers/*.o *.elf *.bin
