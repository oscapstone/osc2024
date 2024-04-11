TARGET = aarch64-unknown-none-softfloat

BUILD_DIR = build
RPI3_DIR = rpi3

KERNEL_ELF = target/$(TARGET)/release/kernel
KERNEL_IMG = $(BUILD_DIR)/kernel8.img

BOOTLOADER_ELF = target/$(TARGET)/release/bootloader
BOOTLOADER_IMG = $(BUILD_DIR)/bootloader.img

PROG_IMG = prog/prog.img

INITRAMFS_CPIO = $(BUILD_DIR)/initramfs.cpio

DTB = $(RPI3_DIR)/bcm2710-rpi-3-b-plus.dtb

CARGO = cargo
CARGO_FLAGS = --release --target=$(TARGET)

# OBJDUMP = aarch64-linux-gnu-objdump
# OBJCOPY = aarch64-linux-gnu-objcopy
OBJDUMP = rust-objdump
OBJCOPY = rust-objcopy

QEMU = qemu-system-aarch64

export dir_guard=@mkdir -p $(@D)

OUTPUT_FILES := $(KERNEL_ELF) $(BOOTLOADER_ELF)
SENTINEL_FILE := .done

.PHONY: all clean run debug size FORCE

all: $(KERNEL_IMG) $(BOOTLOADER_IMG) $(INITRAMFS_CPIO) size

clean:
	$(MAKE) -C prog clean
	$(CARGO) clean
	rm -f $(SENTINEL_FILE)
	rm -rf $(BUILD_DIR)

FORCE:

$(OUTPUT_FILES): $(SENTINEL_FILE)

$(SENTINEL_FILE): FORCE
	$(CARGO) build $(CARGO_FLAGS)
	@touch $(SENTINEL_FILE)

$(KERNEL_IMG): $(KERNEL_ELF)
	$(dir_guard)
	$(OBJCOPY) -O binary $< $@

$(BOOTLOADER_IMG): $(BOOTLOADER_ELF)
	$(dir_guard)
	$(OBJCOPY) -O binary $< $@

$(PROG_IMG): FORCE
	$(MAKE) -C prog

$(INITRAMFS_CPIO): $(wildcard initramfs/*) $(PROG_IMG)
	$(dir_guard)
	cp $(PROG_IMG) initramfs/
	cd initramfs && find . | cpio -o -H newc > ../$@

run: all
	$(QEMU) -M raspi3b \
		-serial null -serial pty \
		-kernel $(BOOTLOADER_IMG) \
		-initrd $(INITRAMFS_CPIO) \
		-dtb $(DTB) 

debug: all size
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD_DIR)/kernel.S
	$(OBJDUMP) -d $(BOOTLOADER_ELF) > $(BUILD_DIR)/bootloader.S
	$(QEMU) -M raspi3b \
		-serial null -serial pty \
		-kernel $(BOOTLOADER_IMG) \
		-initrd $(INITRAMFS_CPIO) \
		-dtb $(DTB) -S -s

size: $(KERNEL_IMG) $(BOOTLOADER_IMG) $(PROG_IMG)
	@stat $(KERNEL_IMG) --printf="Kernel size: %s bytes\n"
	@stat $(BOOTLOADER_IMG) --printf="Bootloader size: %s bytes\n"
	@stat $(PROG_IMG) --printf="Prog size: %s bytes\n"
