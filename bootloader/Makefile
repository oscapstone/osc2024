project_name = bootloader
image_name = bootloader.img

# Define the default target. When you make with no argument, 'build' will be the target.
.PHONY: all
all: build

target/aarch64-unknown-none-softfloat/release/$(project_name):
	cargo rustc --release

$(image_name): target/aarch64-unknown-none-softfloat/release/$(project_name)
	# aarch64-linux-gnu-objcopy -O binary target/aarch64-unknown-none-softfloat/release/$(project_name) $(image_name)
	aarch64-elf-objcopy -O binary target/aarch64-unknown-none-softfloat/release/$(project_name) $(image_name)


# Build the project using Cargo and convert it to a binary format.
.PHONY: build
build: $(image_name)

# Run the project in QEMU.
.PHONY: run
run: build
	qemu-system-aarch64 -M raspi3b -kernel $(image_name) -display none -serial null -serial stdio

# Run the project in QEMU with GDB.
.PHONY: debug
debug: build
	qemu-system-aarch64 -M raspi3b -kernel $(image_name) -display none -serial null -serial stdio -S -s

# Genetate object file
.PHONY: obj
obj: build
	objdump target/aarch64-unknown-none-softfloat/release/$(project_name) -d > obj.txt

.PHONY: clean
clean:
	cargo clean
