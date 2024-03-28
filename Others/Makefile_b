SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: clean kernel8.img

start.o: start.S
	aarch64-none-elf-gcc -c start.S -o start.o

%.o: %.c
	aarch64-none-elf-gcc -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-none-elf-ld start.o $(OBJS) -T linker.ld -o bootloader.elf
	aarch64-none-elf-objcopy -O binary bootloader.elf bootloader.img

clean:
	del bootloader.img bootloader.elf *.o 2>nul || echo off