QEMU_OPTS = -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb

.PHONY: all bootloader kernel run-kernel run-bootloader clean

all: bootloader kernel

bootloader:
	$(MAKE) -C bootloader LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

kernel:
	$(MAKE) -C kernel LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

debug-kernel:
	$(MAKE) -C kernel DEBUG=1 LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

run-kernel:
	$(MAKE) -C kernel run QEMU_OPTS="$(QEMU_OPTS)"

run-debug-kernel:
	$(MAKE) -C kernel run DEBUG=1 QEMU_OPTS="$(QEMU_OPTS)"

run-bootloader:
	$(MAKE) -C bootloader run QEMU_OPTS="$(QEMU_OPTS)"

clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C kernel clean
