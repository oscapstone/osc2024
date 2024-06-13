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
OBJS = $(SRCS:.S=.o)
CFLAGS = -Wall -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles
BOOTLOADER_BIN = kernel8.img
all:  user_program.S
	"aarch64-linux-gnu-gcc" -Wall -O0 -ffreestanding -nostdinc -nostdlib -nostartfiles -c user_program.S -o user_program.o
	"aarch64-linux-gnu-ld" -nostdlib -nostartfiles user_program.o -T link.ld -o user_program.elf
	"aarch64-linux-gnu-objcopy" -O binary user_program.elf user_program.img
