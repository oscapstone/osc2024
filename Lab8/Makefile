SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -g -Wall -O0 -ffreestanding -nostdlib -nostartfiles
all: clean KERNEL8.IMG drive

start.o: start.S
	aarch64-none-elf-gcc $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-none-elf-gcc $(CFLAGS) -c $< -o $@

KERNEL8.IMG: start.o $(OBJS)
	aarch64-none-elf-ld start.o $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-none-elf-objcopy -O binary kernel8.elf KERNEL8.IMG

drive:
	copy sfn_nctuos_fat.img sfn_nctuos.img

clean:
	del KERNEL8.IMG kernel8.elf *.o sfn_nctuos.img 2>nul || echo off

run:
	qemu-system-aarch64 -M raspi3b -kernel KERNEL8.IMG -drive if=sd,file=sfn_nctuos.img,format=raw -serial null -serial stdio -display none -initrd .\initrd.io -dtb .\bcm2710-rpi-3-b-plus.dtb

run_d:
	qemu-system-aarch64 -M raspi3b -kernel KERNEL8.IMG -serial null -serial stdio -initrd .\initramfs.cpio -dtb .\bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel KERNEL8.IMG -serial null -serial stdio -display none -initrd .\initramfs.cpio -dtb .\bcm2710-rpi-3-b-plus.dtb -S -s