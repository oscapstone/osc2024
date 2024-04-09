CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53+nosimd -Iinclude

all: clean user_program.img

start.o: start.S
	clang --target=aarch64-elf $(CFLAGS) -c start.S -o start.o

user_program.img: start.o
	ld.lld -m aarch64elf -static -nostdlib start.o -o user_program.elf
	llvm-objcopy -O binary user_program.elf user_program.img

clean:
	rm user_program.img *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel user_program.img -serial null -serial stdio -display none