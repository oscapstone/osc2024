
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

CPIO = ../initramfs.cpio

CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

LIBRARY_PATH = ../lib
INCLUDES = $(LIBRARY_INCLUDE) -I ./include

LIBRARY_INCLUDE = -I $(LIBRARY_PATH)/include
LIBRARY_SRCS = $(wildcard $(LIBRARY_PATH)/*.c)
LIBRARY_OBJS = $(LIBRARY_SRCS:.c=.o)

all: clean bootloader

start.o: start.S
	$(CC) $(CFLAGS) -c start.S

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

bootloader: start.o $(OBJS) $(LIBRARY_OBJS)
	$(LD) -T linker.ld -o bootloader.elf start.o $(OBJS) $(LIBRARY_OBJS)
	$(OBJCOPY) -O binary bootloader.elf bootloader.img

run:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -initrd $(CPIO) -display none -serial null -serial stdio

debug:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -initrd $(CPIO) -display none -serial null -serial stdio -s -S

clean:
	rm bootloader.elf bootloader.img *.o $(LIBRARY_PATH)/*.o