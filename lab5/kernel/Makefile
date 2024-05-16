TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCPY = $(TOOLCHAIN_PREFIX)objcopy

SRC_DIR = src
SRCS_ASM = $(wildcard $(SRC_DIR)/*.S)
OBJS_ASM = $(SRCS_ASM:%.S=%.o)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:%.c=%.o)

CFLAGS = -Wall -ggdb -mgeneral-regs-only -ffreestanding -nostdlib -I ./include -c

.PHONY: all clean asm run debug

all: kernel8.img

%.o: %.s
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

kernel8.img: $(OBJS) $(OBJS_ASM)
	$(LD) $(OBJS_ASM) $(OBJS) -T $(SRC_DIR)/link.ld -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf kernel8.img

dump: kernel8.img
	aarch64-linux-gnu-objdump -S kernel8.elf > kernel8.dump

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd ../bootloader/initramfs.cpio -dtb ../bootloader/bcm2710-rpi-3-b-plus.dtb -display none -serial null -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd ../bootloader/initramfs.cpio -dtb ../bootloader/bcm2710-rpi-3-b-plus.dtb -display none -S -s

clean:
	rm -f $(SRC_DIR)/*.o kernel8.*
