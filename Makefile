#
# Copyright (C) 2018 bzt (bztsrc@github)
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
#
BUILD_DIR = build
SRC_DIR = src

SRC_C = $(wildcard $(SRC_DIR)/*.c)
SRC_ASM = $(wildcard $(SRC_DIR)/*.S)
SRC_OBJS = $(SRC_C:$(SRC_DIR)/%.c=$(BUILD_DIR)/src/%.o)
SRC_OBJS += $(SRC_ASM:$(SRC_DIR)/%.S=$(BUILD_DIR)/src/%.o)
CFLAGS = -Wall -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles -Iinclude -Ilibrary -c -g

# TOOLCHAIN = aarch64-none-linux-gnu
TOOLCHAIN = aarch64-none-elf
# TOOLCHAIN = /Applications/ArmGNUToolchain/13.2.Rel1/aarch64-none-elf/bin/aarch64-none-elf

.PHONY: all clean

all: clean build_dir kernel8.img

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.S
	mkdir -p $(@D)
	$(TOOLCHAIN)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(TOOLCHAIN)-gcc $(CFLAGS) $< -o $@

initramfs.cpio:
	cd rootfs
	ls . | cpio -o -H newc > ../initramfs.cpio

kernel8.img: $(SRC_OBJS)
	$(TOOLCHAIN)-ld -nostdlib $(SRC_OBJS) -T link.ld -o kernel8.elf
	$(TOOLCHAIN)-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -rf $(BUILD_DIR)/*
	rm *.elf *.img *.o >/dev/null 2>/dev/null || true

run: initramfs.cpio
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio -display none -drive file=test.dd,if=sd,format=raw

dtb: initramfs.cpio
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio

tty:
	qemu-system-aarch64 -M raspi3b -serial null -serial pty

gdb: initramfs.cpio
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio -S -s -display none

sendimg:
	python sendimg.py kernel8.img /dev/cu.usbserial-0001

user_prog:
	$(TOOLCHAIN)-gcc -c user_program.S -o user_program

commu:
	python communicate.py /dev/cu.usbserial-0001


build_dir: $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)