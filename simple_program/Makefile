project_name = simple_program

# Define the default target. When you make with no argument, 'build' will be the target.
.PHONY: all
all: build

target/aarch64-unknown-none-softfloat/release/$(project_name):
	cargo rustc --release

kernel8.img: target/aarch64-unknown-none-softfloat/release/$(project_name)
	# aarch64-linux-gnu-objcopy -O binary target/aarch64-unknown-none-softfloat/release/$(project_name) kernel8.img
	aarch64-elf-objcopy -O binary target/aarch64-unknown-none-softfloat/release/$(project_name) kernel8.img


# Build the project using Cargo and convert it to a binary format.
.PHONY: build
build: kernel8.img

# Run the project in QEMU.
.PHONY: run
run: build
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -initrd ../ramfs/initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb

# Run the project in QEMU with GDB.
.PHONY: debug
debug: build
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -S -s -initrd ../ramfs/initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb

# Genetate object file
.PHONY: obj
obj: build
	objdump target/aarch64-unknown-none-softfloat/release/$(project_name) -d > obj.txt

.PHONY: table
table: build
	objdump target/aarch64-unknown-none-softfloat/release/$(project_name) -t > table.txt

.PHONY: clean
clean:
	cargo clean
