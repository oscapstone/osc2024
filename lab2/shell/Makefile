SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Iinclude -fno-stack-protector -Wall -Wextra -Wpedantic -g  -O2 -ffreestanding -nostdinc -nostdlib -Wno-language-extension-token
CFLAGS += -I../../../opt/lib/gcc/aarch64-linux-gnu/12.2.0/include
all: kernel8.img

start.o: start.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c start.S -o start.o
# clang -mcpu=cortex-a53 --target=aarch64-rpi3-elf $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@
# clang -mcpu=cortex-a53 --target=aarch64-rpi3-elf $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-linux-gnu-ld start.o $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

# ld.lld -m aarch64elf -T linker.ld -o kernel8.elf start.o $(OBJS)
# llvm-objcopy --output-target=aarch64-rpi3-elf -O binary kernel8.elf kernel8.img


clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb