CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53+nosimd

all: clean kernel8.img

start.o: a.S
	clang --target=aarch64-elf $(CFLAGS) -c a.S -o a.o

kernel8.img: start.o
	ld.lld -m aarch64elf -nostdlib a.o -T linker.ld -o kernel8.elf
	llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -d in_asm