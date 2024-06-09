SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -g -Wall -O0 -ffreestanding -nostdlib -nostartfiles
all: clean kernel8.img

start.o: start.S
	aarch64-none-elf-gcc $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-none-elf-gcc $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-none-elf-ld start.o $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-none-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	del kernel8.img kernel8.elf *.o 2>nul || echo off

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -drive if=sd,file=sfn_nctuos.img,format=raw -serial null -serial stdio -display none -initrd .\initrd.io -dtb .\bcm2710-rpi-3-b-plus.dtb

run_d:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -initrd .\initramfs.cpio -dtb .\bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd .\initramfs.cpio -dtb .\bcm2710-rpi-3-b-plus.dtb -S -s