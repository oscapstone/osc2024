SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

all: clean bootloader.img

start.o: start.S
	aarch64-elf-gcc $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

bootloader.img: start.o $(OBJS)
	aarch64-elf-ld -nostdlib -nostartfiles start.o $(OBJS) -T link.ld -o bootloader.elf
	aarch64-elf-objcopy -O binary bootloader.elf bootloader.img

clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial stdio
