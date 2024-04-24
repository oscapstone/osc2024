QEMU_OPTS = -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
BUILD_DIR := $(CURDIR)/build

.PHONY: all bootloader kernel run clean

all: clean kernel run

clean:
	rm -rf $(BUILD_DIR)

bootloader: clean
	$(MAKE) -C bootloader BUILD_DIR=$(BUILD_DIR) LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

kernel: clean
	$(MAKE) -C kernel BUILD_DIR=$(BUILD_DIR) LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

run: 
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/kernel8.img -display none -serial null -serial stdio $(QEMU_OPTS)

run-bootloader: 
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/kernel8.img -display none -serial null -serial pty $(QEMU_OPTS)

debug: 
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/kernel8.img -display none -serial null -serial stdio -s -S $(QEMU_OPTS)
