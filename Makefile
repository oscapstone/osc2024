export TARGET = aarch64-unknown-none-softfloat

BUILD_DIR = build
RPI3_DIR = rpi3

export KERNEL_ELF = target/$(TARGET)/release/kernel
KERNEL_IMG = $(BUILD_DIR)/kernel8.img

export BOOTLOADER_ELF = target/$(TARGET)/release/bootloader
BOOTLOADER_IMG = $(BUILD_DIR)/bootloader.img

INITRAMFS_CPIO = $(BUILD_DIR)/initramfs.cpio

DTB = $(RPI3_DIR)/bcm2710-rpi-3-b-plus.dtb

CARGO = cargo
CARGO_FLAGS = --release --target=$(TARGET)

OBJDUMP = aarch64-linux-gnu-objdump
OBJCOPY = aarch64-linux-gnu-objcopy

QEMU = qemu-system-aarch64

dir_guard=@mkdir -p $(@D)

.PHONY: all clean run debug FORCE

all: $(KERNEL_IMG) $(BOOTLOADER_IMG) $(INITRAMFS_CPIO)
	@stat $(KERNEL_IMG) --printf="Kernel size: %s bytes\n"
	@stat $(BOOTLOADER_IMG) --printf="Bootloader size: %s bytes\n"

clean:
	$(CARGO) clean
	rm -rf $(BUILD_DIR)

FORCE:

$(KERNEL_ELF) $(BOOTLOADER_ELF): FORCE
	$(CARGO) build

$(KERNEL_IMG): $(KERNEL_ELF)
	$(dir_guard)
	$(OBJCOPY) -O binary $< $@

$(BOOTLOADER_IMG): $(BOOTLOADER_ELF)
	$(dir_guard)
	$(OBJCOPY) -O binary $< $@


$(INITRAMFS_CPIO): $(wildcard initramfs/*)
	$(dir_guard)
	cd initramfs && find . | cpio -o -H newc > ../$@

run: all
	$(QEMU) -M raspi3b \
		-serial null -serial pty \
		-kernel $(BOOTLOADER_IMG) \
		-initrd $(INITRAMFS_CPIO) \
		-dtb $(DTB) 

debug: all
	$(QEMU) -M raspi3b -serial null -serial pty -kernel $(BOOTLOADER_IMG) -S -s
