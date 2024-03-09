ARMGNU ?= aarch64-linux-gnu
CFLAGS = -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -Wall -g
QEMUFLAGS = -M raspi3b -display none -serial null -serial pty

all: clean bootloader.img

clean:
	rm -rf build *.img

bootloader.img:
	mkdir -p build
	$(ARMGNU)-gcc $(CFLAGS) -c main.c  -o build/main.o
	$(ARMGNU)-gcc $(CFLAGS) -c start.S -o build/start.o
	$(ARMGNU)-gcc $(CFLAGS) -c boot.c  -o build/boot.o
	$(ARMGNU)-ld -T linker.ld -o build/bootloader.elf build/main.o build/start.o build/boot.o
	$(ARMGNU)-objcopy -O binary build/bootloader.elf bootloader.img

qemu: clean bootloader.img
	qemu-system-aarch64 $(QEMUFLAGS) -kernel bootloader.img -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb