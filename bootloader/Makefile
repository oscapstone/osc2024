BSP = rpi3

TARGET            = aarch64-unknown-none-softfloat
BL_BIN            = bootloader.img

OBJDUMP_BINARY    = aarch64-none-elf-objdump
NM_BINARY         = aarch64-none-elf-mn
READELF_BINARY    = aarch64-none-elf-readelf

LD_SCRIPT_PATH    = $(shell pwd)/src/bsp/raspberrypi
RUSTC_MISC_ARGS   = -C target-cpu=cortex-a53

export LD_SCRIPT_PATH

BL_MANIFEST      = Cargo.toml
BL_LINKER_SCRIPT = bootloader.ld
LAST_BUILD_CONFIG      = target/$(TARGET).build_config

BL_ELF           = ../target/$(TARGET)/release/bootloader

# parses cargo's dep-info file.
BL_ELF_DEPS = $(filter-out %: ,$(file < $(BL_ELF).d)) $(BL_MANIFEST) $(LAST_BUILD_CONFIG)


# command building blocks

RUSTFLAGS = $(RUSTC_MISC_ARGS)														\
						-C link-arg=--library-path=$(LD_SCRIPT_PATH)	\
						-C link-arg=--script=$(BL_LINKER_SCRIPT)


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

all: $(BL_ELF)

$(LAST_BUILD_CONFIG):
	@rm -f target/*.build_config
	@mkdir -p target
	@touch $(LAST_BUILD_CONFIG)

$(BL_ELF): $(BL_ELF_DEPS)
	RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" $(RUSTC_CMD)

clean:
	-rm -r target
	-rm $(BL_BIN)

