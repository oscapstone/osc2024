.PHONY: clean directories out_elf run docker_cp

all: 
	make -C kernel
	make -C bootloader

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel.img -display none -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel.img -display none -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -s -S

run_bootloader:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial pty -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug_bootloader:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial pty -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -s -S

docker_cp:
	docker cp kernel/kernel.elf my_kali:/
	docker cp bootloader/bootloader.elf my_kali:/

cpio:
	cd rootfs && find . | cpio -o -H newc > ../initramfs.cpio

clean:
	rm -rf *.elf
	rm -rf $(BUILD_DIR)
	make -C kernel clean
	make -C bootloader clean