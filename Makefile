BSP = rpi3

TARGET            = aarch64-unknown-none-softfloat

KERNEL_BIN        = kernel8.img
BOOTLOADER_BIN    = bootloader.img

include ./utils/format.mk
include ./utils/os.mk

# QEMU
QEMU_BINARY       = qemu-system-aarch64
QEMU_MACHINE_TYPE = raspi3b
QEMU_RELEASE_ARGS = -display none -serial null -serial stdio
QEMU_DISPLAY_ARGS = -display gtk -serial null -serial stdio
QEMU_DEBUG_ARGS   = -display none -S -s -serial null -serial stdio
QEMU_TTY_ARGS   = -display none -serial null -serial pty
QEMU_TTY_DEBUG_ARGS   = -display none -S -s -serial null -serial pty
EXEC_QEMU = $(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE)

QEMU_DTB_PATH	 = $(shell pwd)/bcm2710-rpi-3-b-plus.dtb

OBJDUMP_BINARY    = aarch64-none-elf-objdump
NM_BINARY         = aarch64-none-elf-mn
READELF_BINARY    = aarch64-none-elf-readelf
RUSTC_MISC_ARGS   = -C target-cpu=cortex-a53

KERNEL_PATH = $(shell pwd)/kernel
BOOTLOADER_PATH = $(shell pwd)/bootloader
INITRD_PATH = $(shell pwd)/initramfs.cpio

KERNEL_ELF           = target/$(TARGET)/release/kernel
BOOTLOADER_ELF	   = target/$(TARGET)/release/bootloader

USER_PROG_IMG = userprog/prog/target/prog
USER_PROG = userprog/prog

OBJCOPY_CMD	 = rust-objcopy \
			--strip-all 	\
			-O binary

.PHONY: all doc qemu clippy clean readelf objdump nm check 

all: $(KERNEL_BIN) $(BOOTLOADER_BIN)

$(KERNEL_BIN): kernel_elf
	$(call color_header, "Generating stripped binary")
	$(OBJCOPY_CMD) $(KERNEL_ELF) $(KERNEL_BIN)
	$(call color_progress_prefix, "Name")
	@echo $(KERNEL_BIN)
	$(call color_progress_prefix, "Size")
	$(call disk_usage_KiB, $(KERNEL_BIN))

$(BOOTLOADER_BIN): bootloader_elf
	$(call color_header, "Generating stripped binary")
	$(OBJCOPY_CMD) $(BOOTLOADER_ELF) $(BOOTLOADER_BIN)
	$(call color_progress_prefix, "Name")
	@echo $(BOOTLOADER_BIN)
	$(call color_progress_prefix, "Size")
	$(call disk_usage_KiB, $(BOOTLOADER_BIN))

kernel_elf:
	make -C $(KERNEL_PATH) all

bootloader_elf:
	make -C $(BOOTLOADER_PATH) all

kernel_gtk: $(KERNEL_BIN) cpio
	$(call color_header, "Launching QEMU")
	$(EXEC_QEMU) $(QEMU_DISPLAY_ARGS) -initrd $(INITRD_PATH) -dtb $(QEMU_DTB_PATH) -kernel $(KERNEL_BIN)

kernel_qemu: $(KERNEL_BIN) cpio
	$(call color_header, "Launching QEMU")
	$(EXEC_QEMU) $(QEMU_RELEASE_ARGS) -initrd $(INITRD_PATH) -dtb $(QEMU_DTB_PATH) -kernel $(KERNEL_BIN) 



kernel_gdb: $(KERNEL_BIN) cpio
	$(call color_header, "Launching QEMU in background")
	$(EXEC_QEMU) $(QEMU_DEBUG_ARGS) -initrd $(INITRD_PATH) -dtb $(QEMU_DTB_PATH) -kernel $(KERNEL_BIN)

bootloader_qemu:$(BOOTLOADER_BIN) $(KERNEL_BIN) cpio
	$(call color_header, "Launching QEMU")
	$(EXEC_QEMU) $(QEMU_TTY_ARGS) -dtb $(QEMU_DTB_PATH) -kernel $(BOOTLOADER_BIN) -initrd $(INITRD_PATH)

bootloader_gdb: $(BOOTLOADER_BIN) $(KERNEL_BIN) cpio 
	$(call color_header, "Launching QEMU in background")
	$(EXEC_QEMU) $(QEMU_TTY_DEBUG_ARGS) $(QEMU_DTB_PATH) -kernel $(BOOTLOADER_BIN) -initrd $(INITRD_PATH) 

$(USER_PROG_IMG): 
	make -C $(USER_PROG) all

cpio: initramfs/* $(USER_PROG_IMG)
	$(call color_header, "Creating initramfs")
	cp $(USER_PROG_IMG) initramfs/
	cd initramfs && find . | cpio -H newc -o > ../initramfs.cpio

clean:
	make -C $(KERNEL_PATH) clean
	make -C $(KERNEL_PATH) clean
	-rm -r target
	-rm $(KERNEL_BIN)
	-rm $(BOOTLOADER_BIN)
	-rm $(INITRD_PATH)
