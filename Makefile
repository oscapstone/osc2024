
BSP = rpi3

TARGET            = aarch64-unknown-none-softfloat
KERNEL_BIN        = kernel8.img
QEMU_BINARY       = qemu-system-aarch64
QEMU_MACHINE_TYPE = raspi3b
QEMU_RELEASE_ARGS = -display none -serial null -serial stdio
QEMU_DEBUG_ARGS   = -display none -S -s -serial null -serial stdio
OBJDUMP_BINARY    = aarch64-none-elf-objdump
NM_BINARY         = aarch64-none-elf-mn
READELF_BINARY    = aarch64-none-elf-readelf
LD_SCRIPT_PATH    = $(shell pwd)/src/bsp/raspberrypi
RUSTC_MISC_ARGS   = -C target-cpu=cortex-a53

export LD_SCRIPT_PATH

KERNEL_MANIFEST      = Cargo.toml
KERNEL_LINKER_SCRIPT = kernel.ld
LAST_BUILD_CONFIG      = target/$(TARGET).build_config

KERNEL_ELF           = target/$(TARGET)/release/kernel

# parses cargo's dep-info file.
KERNEL_ELF_DEPS = $(filter-out %: ,$(file < $(KERNEL_ELF).d)) $(KERNEL_MANIFEST) $(LAST_BUILD_CONFIG)


# command building blocks

RUSTFLAGS = $(RUSTC_MISC_ARGS)														\
						-C link-arg=--library-path=$(LD_SCRIPT_PATH)	\
						-C link-arg=--script=$(KERNEL_LINKER_SCRIPT)

#RUSTFLAGS_PEDANTIC = $(RUSTFLAGS) 			\
#											-D warnings				\
#											-D missing_docs

RUSTFLAGS_PEDANTIC = $(RUSTFLAGS) 			
#											-D warnings				\
#											-D missing_docs

FEATURE    = --features bsp_$(BSP)
COMPILER_ARGS = --target=$(TARGET) \
							 $(FEATURES)        \
							 --release

RUSTC_CMD    = cargo rustc $(COMPILER_ARGS)
CLIPPY_CMD   = cargo clippy $(COMPILER_ARGS)
OBJCOPY_CMD	 = rust-objcopy \
							 --strip-all 	\
							 -O binary

EXEC_QEMU = $(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE)

.PHONY: all doc qemu clippy clean readelf objdump nm check

all: $(KERNEL_BIN)

# Save the configuration as a file

$(LAST_BUILD_CONFIG):
	@rm -f target/*.build_config
	@mkdir -p target
	@touch $(LAST_BUILD_CONFIG)

$(KERNEL_ELF): $(KERNEL_ELF_DEPS)
	RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" $(RUSTC_CMD)

# -- -C panic=abort  -C panic=abort -C opt-level=3

$(KERNEL_BIN):$(KERNEL_ELF)
	$(call color_header, "Generating stripped binary")
	@$(OBJCOPY_CMD) $(KERNEL_ELF) $(KERNEL_BIN)
	$(call color_progress_prefix, "Name")
	@echo $(KERNEL_BIN)
	$(call color_progress_prefix, "Size")
	$(call disk_usage_KiB, $(KERNEL_BIN))

qemu: $(KERNEL_BIN)
	$(call color_header, "Launching QEMU")
	$(EXEC_QEMU) $(QEMU_RELEASE_ARGS) -kernel $(KERNEL_BIN)

gdb: $(KERNEL_BIN)
	$(call color_header, "Launching QEMU in background")
	$(EXEC_QEMU) $(QEMU_DEBUG_ARGS) -kernel $(KERNEL_BIN)
	
clean:
	rm -r target
	rm $(KERNEL_BIN)

