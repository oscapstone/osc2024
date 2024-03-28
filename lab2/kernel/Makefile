TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCPY = $(TOOLCHAIN_PREFIX)objcopy

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:%.c=%.o)

CFLAGS = -Wall -O2 -ffreestanding -nostdinc -I ./include -c

.PHONY: all clean asm run debug

all: kernel8.img

$(SRC_DIR)/start.o: $(SRC_DIR)/start.s
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

kernel8.img: $(OBJS) $(SRC_DIR)/start.o
	$(LD) $(SRC_DIR)/start.o $(OBJS) -T $(SRC_DIR)/link.ld -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf kernel8.img

asm:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd ../bootloader/initramfs.cpio -dtb ../bootloader/bcm2710-rpi-3-b-plus.dtb -display none -serial null -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	rm -f $(SRC_DIR)/*.o kernel8.*
