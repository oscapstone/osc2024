TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCPY = $(TOOLCHAIN_PREFIX)objcopy

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:%.c=%.o)

CFLAGS = -Wall -ffreestanding -nostdinc -I ./include -c

.PHONY: all clean asm run debug

all: bootloader.img

$(SRC_DIR)/start.o: $(SRC_DIR)/start.s
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

bootloader.img: $(OBJS) $(SRC_DIR)/start.o
	$(LD) $(SRC_DIR)/start.o $(OBJS) -T $(SRC_DIR)/link.ld -o bootloader.elf
	$(OBJCPY) -O binary bootloader.elf bootloader.img

run: all
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -initrd initramfs.cpio -display none -serial null -serial pty

debug: all
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -S -s

clean:
	rm -f $(SRC_DIR)/*.o bootloader.*
