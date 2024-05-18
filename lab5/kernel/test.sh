#! /bin/bash
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -dtb ../../bcm2710-rpi-3-b-plus.dtb -initrd ../../initramfs.cpio 
