# Define the default target. When you make with no argument, 'build' will be the target.
.PHONY: all
all: build

target/aarch64-unknown-none-softfloat/release/testing:
	cargo rustc --release --target aarch64-unknown-none-softfloat

kernel8.img: target/aarch64-unknown-none-softfloat/release/testing
	aarch64-linux-gnu-objcopy -O binary target/aarch64-unknown-none-softfloat/release/testing kernel8.img

# Build the project using Cargo and convert it to a binary format.
.PHONY: build
build: kernel8.img

# Run the project in QEMU.
.PHONY: run
run: build
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

# Run the project in QEMU with GDB.
.PHONY: debug
debug: build
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -S -s

# Genetate object file
.PHONY: obj
obj: build
	objdump target/aarch64-unknown-none-softfloat/release/testing -d > obj.txt

.PHONY: clean
clean:
	cargo clean
