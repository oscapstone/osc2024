port=/dev/ttyUSB0
ver=release

.PHONY: all rootfs emu send screen clean

all:
	make -C bootloader ver=$(ver)
	make -C kernel ver=$(ver)

rootfs: 
	$(shell cd rootfs && find . | cpio -o -H newc > ../initramfs.cpio && cd ..)

emu:
	qemu-system-aarch64 -M raspi3b -kernel bootloader/bootloader.img -display none -serial null -serial pty -initrd initramfs.cpio &

send:
	sudo chmod 666 $(port)
	. ../.OSC/bin/activate
	python3 bootloader/src/send_img.py --device $(port)

screen:
	screen $(port) 115200

clean:
	make -C bootloader clean
	make -C kernel clean
