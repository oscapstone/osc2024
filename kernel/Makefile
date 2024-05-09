## SPDX-License-Identifier: MIT OR Apache-2.0
##
## Copyright (c) 2018-2023 Andre Richter <andre.o.richter@gmail.com>
##
## This file is part of the `rust-raspberrypi-OS-tutorials` project.
## https://github.com/rust-embedded/rust-raspberrypi-OS-tutorials/
##
##--------------------------------------------------------------------------------------------------
## Optional, user-provided configuration values
##--------------------------------------------------------------------------------------------------

# BSP ?= rpi3
TARGET            = aarch64-unknown-none-softfloat
KERNEL_BIN        = kernel8.img
QEMU_BINARY       = qemu-system-aarch64
QEMU_MACHINE_TYPE = raspi3
QEMU_RELEASE_ARGS = -display none -d in_asm

# LD_SCRIPT_PATH    = src/
# LD_SCRIPT_NAME    = linker.ld
LD_SCRIPT_NAME    = kernel.ld

# LD_SCRIPT_PATH    = $(shell pwd)/src/bsp/raspberrypi
LD_SCRIPT_PATH    = src/
KERNEL_LINKER_SCRIPT = $(LD_SCRIPT_PATH)bsp/rpi3/$(LD_SCRIPT_NAME)


RUSTC_MISC_ARGS   = -C target-cpu=cortex-a53

# Export for build.rs.
export LD_SCRIPT_PATH



##--------------------------------------------------------------------------------------------------
## Targets and Prerequisites
##--------------------------------------------------------------------------------------------------
KERNEL_MANIFEST      = Cargo.toml
# KERNEL_LINKER_SCRIPT = kernel.ld
LAST_BUILD_CONFIG    = target/$(BSP).build_config

KERNEL_ELF			= target/$(TARGET)/debug/kernel
# KERNEL_ELF      = target/$(TARGET)/release/kernel
# This parses cargo's dep-info file.
# https://doc.rust-lang.org/cargo/guide/build-cache.html#dep-info-files
KERNEL_ELF_DEPS = $(filter-out %: ,$(file < $(KERNEL_ELF).d)) $(KERNEL_MANIFEST) $(LAST_BUILD_CONFIG)



##--------------------------------------------------------------------------------------------------
## Command building blocks
##--------------------------------------------------------------------------------------------------
RUSTFLAGS = $(RUSTC_MISC_ARGS)                   \
    -C link-arg=--library-path=$(LD_SCRIPT_PATH) \
    -C link-arg=--script=$(KERNEL_LINKER_SCRIPT)

RUSTFLAGS_PEDANTIC = $(RUSTFLAGS) \
    -D warnings                   \
    -D missing_docs

# FEATURES      = --features bsp_$(BSP)
COMPILER_ARGS = --target=$(TARGET) \
# --release
    # $(FEATURES)\                    
    

RUSTC_CMD   = cargo rustc $(COMPILER_ARGS)
# RUSTC_CMD   = cargo miri $(COMPILER_ARGS)
OBJCOPY_CMD = rust-objcopy \
    --strip-all            \
    -O binary

##--------------------------------------------------------------------------------------------------
## Targets
##--------------------------------------------------------------------------------------------------

all: $(KERNEL_BIN)

##------------------------------------------------------------------------------
## Save the configuration as a file, so make understands if it changed.
##------------------------------------------------------------------------------
$(LAST_BUILD_CONFIG):
	@rm -f target/*.build_config
	@mkdir -p target
	@touch $(LAST_BUILD_CONFIG)

##------------------------------------------------------------------------------
## Compile the kernel ELF
##------------------------------------------------------------------------------
$(KERNEL_ELF): $(KERNEL_ELF_DEPS)
# $(call color_header, "Compiling kernel ELF - $(BSP)")
	@RUSTFLAGS="$(RUSTFLAGS)" $(RUSTC_CMD)

##------------------------------------------------------------------------------
## Generate the stripped kernel binary
##------------------------------------------------------------------------------
$(KERNEL_BIN): $(KERNEL_ELF)
	$(call color_header, "Generating stripped binary")
	@$(OBJCOPY_CMD) $(KERNEL_ELF) $(KERNEL_BIN)
	$(call color_progress_prefix, "Name")
	@echo $(KERNEL_BIN)
	$(call color_progress_prefix, "Size")
	$(call disk_usage_KiB, $(KERNEL_BIN))


##------------------------------------------------------------------------------
## Debug
##------------------------------------------------------------------------------
debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img \
	-display none -serial null -serial stdio \
	-dtb ../resources/bcm2710-rpi-3-b-plus.dtb \
	-initrd ../resources/initramfs.cpio

##------------------------------------------------------------------------------
## Clean
##------------------------------------------------------------------------------
clean:
	rm -rf target $(KERNEL_BIN)
