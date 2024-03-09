ARMGNU ?= aarch64-linux-gnu
CFLAGS =  -I../include -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -Wall -g
ASMFLAGS = -I../include
QEMUFLAGS = -M raspi3b -display none -serial null -serial pty

all: clean bootloader.img

clean:
	rm -rf build *.img

bootloader.img:
	mkdir -p build
	$(ARMGNU)-gcc $(CFLAGS) -c main.c         -o build/main.o
	$(ARMGNU)-gcc $(CFLAGS) -c start.S        -o build/start.o
	$(ARMGNU)-gcc $(CFLAGS) -c ../src/uart.c  -o build/uart.o
	$(ARMGNU)-gcc $(CFLAGS) -c ../src/utils.c -o build/utils.o
	$(ARMGNU)-ld -T linker.ld -o build/bootloader.elf build/main.o build/start.o build/uart.o build/utils.o
	$(ARMGNU)-objcopy -O binary build/bootloader.elf bootloader.img

qemu: clean bootloader.img
	qemu-system-aarch64 $(QEMUFLAGS) -kernel bootloader.img -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb