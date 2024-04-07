TARGET_DIR := ../../target

TARGET_TRIPLE := aarch64-unknown-none-softfloat
TARGET_CPU    := cortex-a53

UARTLOAD_ELF := $(TARGET_DIR)/$(TARGET_TRIPLE)/release/uartload
UARTLOAD_IMG := $(TARGET_DIR)/$(TARGET_TRIPLE)/release/kernel8.img
UARTLOAD_DEP_INFO := $(filter-out %: ,$(file < $(UARTLOAD_ELF).d)) 

LD_SCRIPT_PATH := $(shell pwd)
LD_SCRIPT_NAME := uartload.ld

RUSTFLAGS := -C target-cpu=$(TARGET_CPU) \
	-C link-arg=--library-path=$(LD_SCRIPT_PATH) \
	-C link-arg=--script=$(LD_SCRIPT_NAME)
RUSTFLAGS_PEDANTIC := $(RUSTFLAGS) -D warnings
RUSTC_ARGS := --target=$(TARGET_TRIPLE) --release

CPIO_ARCHIVE := ../kernel/initramfs.cpio
DTB_FILE := ../kernel/bcm2710-rpi-3-b-plus.dtb

QEMU_BINARY := qemu-system-aarch64
QEMU_MACHINE_TYPE := raspi3b
QEMU_RELEASE_ARGS := -serial null -serial pty -display none -initrd $(CPIO_ARCHIVE) -dtb $(DTB_FILE)

# export for build.rs
export LD_SCRIPT_PATH

.PHONY: all
all: $(UARTLOAD_IMG)

$(UARTLOAD_IMG): $(UARTLOAD_ELF)
	@echo "Generating stripped binary"
	@rust-objcopy --strip-all -O binary $< $@
	@echo "Name: $@"
	@echo "Size: `du --block-size=1024 --apparent-size $@ | cut -f1`"

$(UARTLOAD_ELF): $(UARTLOAD_DEP_INFO) Cargo.toml
	@echo "Compiling UARTLOAD ELF"
	@RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" cargo rustc $(RUSTC_ARGS)

.PHONY: qemu
qemu: $(UARTLOAD_IMG)
	@echo "Launching QEMU"
	@$(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE) $(QEMU_RELEASE_ARGS) -kernel $<

.PHONY: qemu-debug
qemu-debug: $(UARTLOAD_IMG)
	@echo "Launching QEMU in debug mode"
	@$(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE) $(QEMU_RELEASE_ARGS) -kernel $< -S -s

.PHONY: clippy
clippy:
	@RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" cargo clippy $(RUSTC_ARGS)

