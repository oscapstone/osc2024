TARGET = aarch64-unknown-none-softfloat

BUILD_DIR = build
RPI3_DIR = rpi3
INITRAMFS_DIR = initramfs

KERNEL_ELF = target/$(TARGET)/release/kernel
KERNEL_IMG = $(BUILD_DIR)/kernel8.img

BOOTLOADER_ELF = target/$(TARGET)/release/bootloader
BOOTLOADER_IMG = $(BUILD_DIR)/bootloader.img

CPROG = $(INITRAMFS_DIR)/cprog.img
CPROG_IMG = prog/prog.img

RPROG = $(INITRAMFS_DIR)/rprog.img
RPROG_ELF = target/$(TARGET)/release/program
RPROG_IMG = $(BUILD_DIR)/program.img

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

OUTPUT_ELFS := $(KERNEL_ELF) $(BOOTLOADER_ELF)
SENTINEL_FILE := .done

.PHONY: all clean run debug debug-qemu size FORCE

all: $(KERNEL_IMG) $(BOOTLOADER_IMG) $(INITRAMFS_CPIO) size

clean:
	$(MAKE) -C prog clean
	$(CARGO) clean
	rm -f $(SENTINEL_FILE)
	rm -rf $(BUILD_DIR)
	rm -f $(CPROG)
	rm -f $(RPROG)

FORCE:

$(OUTPUT_ELFS): $(SENTINEL_FILE)

$(SENTINEL_FILE): FORCE
	$(CARGO) build $(CARGO_FLAGS)
	@touch $(SENTINEL_FILE)

$(KERNEL_IMG): $(KERNEL_ELF) FORCE
	$(dir_guard)
	$(OBJCOPY) -O binary $< $@

$(BOOTLOADER_IMG): $(BOOTLOADER_ELF) FORCE
	$(dir_guard)
	$(OBJCOPY) -O binary $< $@

$(RPROG_IMG): $(RPROG_ELF) FORCE
	$(OBJCOPY) -O binary $< $@

$(RPROG): $(RPROG_IMG)
	$(dir_guard)
	cp $(RPROG_IMG) $@

$(CPROG_IMG):
	$(MAKE) -C prog

$(CPROG): $(CPROG_IMG)
	$(dir_guard)
	cp $(CPROG_IMG) $@

$(INITRAMFS_CPIO): $(CPROG) $(RPROG)
	$(dir_guard)
	cd initramfs && find . | cpio -o -H newc > ../$@

run: all
	$(QEMU) -M raspi3b \
		-serial null -serial pty \
		-kernel $(BOOTLOADER_IMG) \
		-initrd $(INITRAMFS_CPIO) \
		-dtb $(DTB) --daemonize

debug: all size $(CPROG) $(RPROG)
	$(OBJDUMP) -D $(KERNEL_ELF) > $(BUILD_DIR)/kernel.S
	$(OBJDUMP) -D $(BOOTLOADER_ELF) > $(BUILD_DIR)/bootloader.S
	$(OBJDUMP) -D $(RPROG_ELF) > $(BUILD_DIR)/rprog.S

debug-qemu:
	$(QEMU) -M raspi3b \
		-serial null -serial pty \
		-kernel $(BOOTLOADER_IMG) \
		-initrd $(INITRAMFS_CPIO) \
		-dtb $(DTB) -S -s

size: $(KERNEL_IMG) $(BOOTLOADER_IMG)
	@printf "Kernel: %d (0x%x) bytes\n" `stat -c %s $(KERNEL_IMG)` `stat -c %s $(KERNEL_IMG)`
	@printf "Bootloader: %d (0x%x) bytes\n" `stat -c %s $(BOOTLOADER_IMG)` `stat -c %s $(BOOTLOADER_IMG)`
	@printf "Initramfs: %d (0x%x) bytes\n" `stat -c %s $(INITRAMFS_CPIO)` `stat -c %s $(INITRAMFS_CPIO)`
