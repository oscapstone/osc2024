include ./format.mk

BSP			  ?= rpi3

TARGET            = aarch64-unknown-none-softfloat
KERNEL_BIN        = kernel8.img
QEMU_BINARY       = qemu-system-aarch64
QEMU_MACHINE_TYPE = raspi3b
QEMU_RELEASE_ARGS = -d in_asm -display none
OBJDUMP_BINARY    = aarch64-none-elf-objdump
NM_BINARY         = aarch64-none-elf-nm
READELF_BINARY    = aarch64-none-elf-readelf
LD_SCRIPT_PATH    = $(shell pwd)/src
RUSTC_MISC_ARGS   = -C target-cpu=cortex-a53

KERNEL_MANIFEST      = Cargo.toml
KERNEL_LINKER_SCRIPT = linker.ld
LAST_BUILD_CONFIG    = target/$(BSP).build_config

KERNEL_ELF      = target/$(TARGET)/release/kernel
# This parses cargo's dep-info file.
# https://doc.rust-lang.org/cargo/guide/build-cache.html#dep-info-files
KERNEL_ELF_DEPS = $(filter-out %: ,$(file < $(KERNEL_ELF).d)) $(KERNEL_MANIFEST) $(LAST_BUILD_CONFIG)

export LD_SCRIPT_PATH

## Command building blocks
RUSTFLAGS = $(RUSTC_MISC_ARGS)                   \
    -C link-arg=--library-path=$(LD_SCRIPT_PATH) \
    -C link-arg=--script=$(KERNEL_LINKER_SCRIPT)

RUSTFLAGS_PEDANTIC = $(RUSTFLAGS) \
    # -D warnings                   \
    -D missing_docs

FEATURES      = --features bsp_$(BSP)
COMPILER_ARGS = --target=$(TARGET) \
	$(FEATURES) \
    --release

RUSTC_CMD   = cargo rustc $(COMPILER_ARGS)
DOC_CMD     = cargo doc $(COMPILER_ARGS)
OBJCOPY_CMD = rust-objcopy \
    --strip-all            \
    -O binary

EXEC_QEMU = $(QEMU_BINARY) -M $(QEMU_MACHINE_TYPE)


## Make targets
.PHONY: all doc qemu clean 

all: $(KERNEL_BIN)

$(LAST_BUILD_CONFIG):
	#@rm -f target/*.build_conqemu-system-aarch64 -M raspi3b -display none -d in_asm -kernel kernel8.img -serial null -serial stdiofig
	@mkdir -p target
	@touch $(LAST_BUILD_CONFIG)

## Compile the kernel ELF
$(KERNEL_ELF): $(KERNEL_ELF_DEPS)
	$(call color_header, "Compiling kernel ELF")
	@echo "Executing: RUSTFLAGS=\"$(RUSTFLAGS_PEDANTIC)\" $(RUSTC_CMD)"
	@RUSTFLAGS="$(RUSTFLAGS_PEDANTIC)" $(RUSTC_CMD)

## Generate the stripped kernel binary
$(KERNEL_BIN): $(KERNEL_ELF)
	$(call color_header, "Generating stripped binary")
	@echo "Executing:  $(OBJCOPY_CMD) $(KERNEL_ELF) $(KERNEL_BIN)"
	@$(OBJCOPY_CMD) $(KERNEL_ELF) $(KERNEL_BIN)
	$(call color_progress_prefix, "Name")
	@echo $(KERNEL_BIN)
	$(call color_progress_prefix, "Size")
	$(call disk_usage_KiB, $(KERNEL_BIN))

doc:
	$(call color_header, "Generating docs")
	@$(DOC_CMD) --document-private-items --open


ifeq ($(QEMU_MACHINE_TYPE),) # QEMU is not supported for the board.
qemu-system-aarch64 -M raspi3b -display none -d in_asm -kernel kernel8.img -serial null -serial stdio
qemu:
	$(call color_header, "$(QEMU_MISSING_STRING)")

else # QEMU is supported.

qemu: $(KERNEL_BIN)
	$(call color_header, "Launching QEMU")
	$(EXEC_QEMU) $(QEMU_RELEASE_ARGS) -kernel $(KERNEL_BIN)
endif

clean: 
	rm -rf target $(KERNEL_BIN)

run: all
	qemu-system-aarch64 -M raspi3b -display none -d in_asm -kernel kernel8.img -serial null -serial stdio

## TODO: 翔程的東西，之後看功能是什麼
debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -s -S
