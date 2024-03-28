
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

CPIO = ../initramfs.cpio
DTB = ../bcm2710-rpi-3-b-plus.dtb

CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

LIBRARY_PATH = ../lib
INCLUDES = -I ./include \
	$(LIBRARY_INCLUDE)

LIBRARY_INCLUDE = -I $(LIBRARY_PATH)/include
LIBRARY_SRCS = $(wildcard $(LIBRARY_PATH)/*.c)
LIBRARY_OBJS = $(LIBRARY_SRCS:.c=.o)

all: clean kernel

start.o: start.S
	$(CC) $(CFLAGS) -c start.S

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

kernel: start.o $(OBJS) $(LIBRARY_OBJS)
	$(LD) -T linker.ld -o kernel8.elf start.o $(OBJS) $(LIBRARY_OBJS)
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd $(CPIO) -display none -serial null -serial stdio -dtb $(DTB)
debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd $(CPIO) -display none -serial null -serial stdio -s -S

clean:
	rm kernel8.elf kernel8.img *.o