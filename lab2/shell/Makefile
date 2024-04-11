SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Iinclude -fno-stack-protector -Wall -Wextra -Wpedantic -Werror -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles 
# CFLAGS += -I../../../opt/lib/gcc/aarch64-linux-gnu/11.4.0/include
CFLAGS += -I /usr/aarch64-linux-gnu/include

all: shell_kernel.img

start.o: start.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

shell_kernel.img: start.o $(OBJS)
	aarch64-linux-gnu-ld start.o $(OBJS) -T linker.ld -o shell_kernel.elf
	aarch64-linux-gnu-objcopy -O binary shell_kernel.elf shell_kernel.img

clean:
	rm shell_kernel.elf *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb  -display none  -kernel shell_kernel.img