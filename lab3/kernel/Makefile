SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -nostdlib -ffreestanding -nostdinc -mcpu=cortex-a53+nosimd -Iinclude 

all: clean kernel8.img

start.o: src/start.S
	clang --target=aarch64-elf $(CFLAGS) -c src/start.S -o src/start.o

%.o: %.c
	clang --target=aarch64-elf $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	ld.lld -m aarch64elf -nostdlib src/start.o $(OBJS) -T src/link.ld -o kernel8.elf
	llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.img kernel8.elf src/*.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb