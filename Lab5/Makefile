port=/dev/ttyUSB0
image=kernel/kernel8.img
ver=release

.PHONY: all rootfs emu send screen clean kill

all:
	make -C bootloader ver=$(ver)
	make -C kernel ver=$(ver)
	make -C user_program

rootfs: 
	$(shell cp user_program/prog1.img rootfs/)
	$(shell cd rootfs && find . | cpio -o -H newc > ../initramfs.cpio && cd ..)

emu:
	qemu-system-aarch64 -M raspi3b -kernel bootloader/bootloader.img -display none -serial null -serial pty -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb &

send:
	sudo chmod 666 $(port)
	. ../.OSC/bin/activate
	python3 bootloader/send_img.py --device $(port) --image $(image)

screen:
	sudo screen $(port) 115200

minicom:
	sudo minicom --device=$(port) --baudrate=115200

clean:
	make -C bootloader clean
	make -C kernel clean
	make -C user_program clean

kill:
	killall qemu-system-aarch64
