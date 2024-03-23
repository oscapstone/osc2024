BSP = rpi3

TARGET            = aarch64-unknown-none-softfloat
KERNEL_BIN        = kernel8.img
# QEMU
QEMU_BINARY       = qemu-system-aarch64
QEMU_MACHINE_TYPE = raspi3b
QEMU_RELEASE_ARGS = -display none -serial null -serial stdio
QEMU_DEBUG_ARGS   = -display none -S -s -serial null -serial stdio
EXEC_QEMU = $(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE)

OBJDUMP_BINARY    = aarch64-none-elf-objdump
NM_BINARY         = aarch64-none-elf-mn
READELF_BINARY    = aarch64-none-elf-readelf
RUSTC_MISC_ARGS   = -C target-cpu=cortex-a53



KERNEL_PATH = $(shell pwd)/kernel

KERNEL_ELF           = target/$(TARGET)/release/kernel
BOOTLOADER_ELF	   = target/$(TARGET)/release/bootloader

OBJCOPY_CMD	 = rust-objcopy \
			--strip-all 	\
			-O binary

.PHONY: all doc qemu clippy clean readelf objdump nm check 

all: $(KERNEL_BIN)

$(KERNEL_BIN): kernel_elf
	$(call color_header, "Generating stripped binary")
	@$(OBJCOPY_CMD) $(KERNEL_ELF) $(KERNEL_BIN)
	$(call color_progress_prefix, "Name")
	@echo $(KERNEL_BIN)
	$(call color_progress_prefix, "Size")
	$(call disk_usage_KiB, $(KERNEL_BIN))

kernel_elf:
	make -C $(KERNEL_PATH) all

qemu: $(KERNEL_BIN)
	$(call color_header, "Launching QEMU")
	$(EXEC_QEMU) $(QEMU_RELEASE_ARGS) -kernel $(KERNEL_BIN)

gdb: $(KERNEL_BIN)
	$(call color_header, "Launching QEMU in background")
	$(EXEC_QEMU) $(QEMU_DEBUG_ARGS) -kernel $(KERNEL_BIN)

clean:
	make -C $(KERNEL_PATH) clean
	-rm -r target
	-rm $(KERNEL_BIN)
