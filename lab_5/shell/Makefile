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

GCC = "aarch64-linux-gnu-gcc"
LD = "aarch64-linux-gnu-ld"
OBJCPY = "aarch64-linux-gnu-objcopy"
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles
BOOTLOADER_BIN = kernel8.img
all: clean kernel8.img

# start.o: start.S
# 	$(GCC) $(CFLAGS) -c start.S -o start.o

%.o: %.S
	$(GCC) $(CFLAGS) -c $< -o $@ -g
	"aarch64-linux-gnu-gcc" -Wall -w -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles -c exception_vector.S -o exception_vector.o -g
	"aarch64-linux-gnu-gcc" -Wall -w -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles -c interrupts.S -o interrupts.o -g
	"aarch64-linux-gnu-gcc" -Wall -w -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles -c _thread.S -o _thread.o -g

%.o: %.c
	$(GCC) $(CFLAGS) -c $< -w -o $@ -g


kernel8.img: start.o $(OBJS)
	$(LD) -nostdlib -nostartfiles start.o $(OBJS) exception_vector.o interrupts.o _thread.o -T link.ld -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf shell_kernel.img

clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true

run_asm:
	qemu-system-aarch64 -M raspi3b -kernel shell_kernel.img -initrd initramfs.cpio -serial null -serial stdio -display none -d in_asm

run:
	qemu-system-aarch64 -M raspi3b -kernel shell_kernel.img -initrd initramfs.cpio -serial null -serial stdio -display none

debug:
	qemu-system-aarch64 -M raspi3b -kernel shell_kernel.img -initrd initramfs.cpio -serial null -serial stdio -display none -s -S
