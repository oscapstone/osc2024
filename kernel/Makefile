
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

KERNEL_ELF           = ../target/$(TARGET)/release/kernel

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

FEATURES    = --features bsp_$(BSP)
COMPILER_ARGS = --target=$(TARGET) \
				$(FEATURES)        \
				--release

RUSTC_CMD    = cargo rustc $(COMPILER_ARGS) --bin kernel
CLIPPY_CMD   = cargo clippy $(COMPILER_ARGS)


.PHONY: all doc qemu clippy clean readelf objdump nm check

all: $(KERNEL_ELF)

$(LAST_BUILD_CONFIG):
	@rm -f target/*.build_config
	@mkdir -p target
	@touch $(LAST_BUILD_CONFIG)

$(KERNEL_ELF): $(KERNEL_ELF_DEPS)
	RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" $(RUSTC_CMD)

# -- -C panic=abort  -C panic=abort -C opt-level=3

	
clean:
	-rm -r target
	-rm $(KERNEL_BIN)

