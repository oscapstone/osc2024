TARGET_DIR := ../../target

TARGET_TRIPLE := aarch64-unknown-none-softfloat
TARGET_CPU    := cortex-a53

KERNEL_ELF := $(TARGET_DIR)/$(TARGET_TRIPLE)/release/kernel
KERNEL_IMG := $(TARGET_DIR)/$(TARGET_TRIPLE)/release/kernel8.img
KERNEL_DEP_INFO := $(filter-out %: ,$(file < $(KERNEL_ELF).d)) 

LD_SCRIPT_PATH := $(shell pwd)
LD_SCRIPT_NAME := kernel.ld

RUSTFLAGS := -C target-cpu=$(TARGET_CPU) \
	-C link-arg=--library-path=$(LD_SCRIPT_PATH) \
	-C link-arg=--script=$(LD_SCRIPT_NAME)
RUSTFLAGS_PEDANTIC := $(RUSTFLAGS) -D warnings
RUSTC_ARGS := --target=$(TARGET_TRIPLE) --release

QEMU_BINARY := qemu-system-aarch64
QEMU_MACHINE_TYPE := raspi3b
QEMU_RELEASE_ARGS := -d in_asm -display none

# export for build.rs
export LD_SCRIPT_PATH

.PHONY: all
all: $(KERNEL_IMG)

$(KERNEL_IMG): $(KERNEL_ELF)
	@echo "Generating stripped binary"
	@rust-objcopy --strip-all -O binary $(KERNEL_ELF) $(KERNEL_IMG)
	@echo "Name: $(KERNEL_IMG)"
	@echo "Size: `du --block-size=1024 --apparent-size $(KERNEL_IMG) | cut -f1`"

$(KERNEL_ELF): $(KERNEL_DEP_INFO) Cargo.toml
	@echo "Compiling kernel ELF"
	@RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" cargo rustc $(RUSTC_ARGS)

.PHONY: qemu
qemu: $(KERNEL_IMG)
	@echo "Launching QEMU"
	@$(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE) $(QEMU_RELEASE_ARGS) -kernel $(KERNEL_IMG)

.PHONY: clippy
clippy:
	@RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" cargo clippy $(RUSTC_ARGS)

