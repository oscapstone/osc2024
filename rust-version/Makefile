## SPDX-License-Identifier: MIT OR Apache-2.0
##
## Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>
# This file is part of the `rust-raspberrypi-OS-tutorials` project.
#
################################################################################

# this would build according to the rust target triple
# https://clang.llvm.org/docs/CrossCompilation.html#target-triple
TARGET                    = aarch64-unknown-none-softfloat
KERNEL_BIN                = kernel8.img
BOOT_BIN                  = bootloader.img
QEMU_BINARY               = qemu-system-aarch64
QEMU_MACHINE_TYPE         = raspi3
QEMU_RELEASE_ARGS         = -d in_asm -display none
RUSTC_MISC_ARGS           = -C target-cpu=cortex-a53
KERNEL_LINKER_SCRIPT      = src/linker.ld
BOOTLOADER_LINKER_SCRIPT  = src/bootloader/linker.ld
KERNEL_RUSTFLAGS          = $(RUSTC_MISC_ARGS)   \
    -C link-arg=--library-path=$(LD_SCRIPT_PATH) \
    -C link-arg=--script=$(KERNEL_LINKER_SCRIPT)
BOOTLOADER_RUSTFLAGS      = $(RUSTC_MISC_ARGS)   \
		-C link-arg=--library-path=$(BOOTLOADER_LD_SCRIPT_PATH) \
		-C link-arg=--script=$(BOOTLOADER_LINKER_SCRIPT)


# build my kernel
KERNEL_ELF = target/$(TARGET)/debug/kernel
KERNEL_ELF_DEPS = $(filter-out %: ,$(file < $(KERNEL_ELF).d)) Cargo.toml

COMPILER_ARGS = --target=$(TARGET)

all: $(KERNEL_BIN)

$(KERNEL_ELF): $(KERNEL_ELF_DEPS)
	@RUSTFLAGS="$(KERNEL_RUSTFLAGS)" cargo rustc $(COMPILER_ARGS)

$(KERNEL_BIN): $(KERNEL_ELF)
	rust-objcopy -O binary $(KERNEL_ELF) $(KERNEL_BIN)

$(BOOT_ELF): $(BOOT_ELF_DEPS)
	@RUSTFLAGS="$(BOOTLOADER_RUSTFLAGS)" cargo rustc $(COMPILER_ARGS) --bin bootloader

$(BOOT_BIN): $(BOOT_ELF)
	rust-objcopy -O binary $(BOOT_ELF) $(BOOT_BIN)

testboot: $(KERNEL_BIN) $(BOOT_BIN)
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial pty

test: $(KERNEL_BIN) $(BOOT_BIN)
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -initrd initramfs.cpio

clean:
	rm -rf target $(KERNEL_BIN)

# debugging
readelf:
	readelf --header $(KERNEL_ELF)

objdump:
	rust-objdump --disassemble --demangle \
		--section .text --section .rodata \
		$(KERNEL_ELF) | rustfilt | less

nm:
	nm --demangle --print-size $(KERNEL_ELF) | less

