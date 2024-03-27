SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a53+nosimd -Iinclude

all: clean bootloader.img

start.o: src/start.S
	clang --target=aarch64-elf $(CFLAGS) -c src/start.S -o src/start.o

%.o: %.c
	clang --target=aarch64-elf $(CFLAGS) -c $< -o $@

bootloader.img: start.o $(OBJS)
	ld.lld -m aarch64elf -nostdlib src/start.o $(OBJS) -T src/link.ld -o bootloader.elf
	llvm-objcopy -O binary bootloader.elf bootloader.img

clean:
	rm bootloader.img bootloader.elf src/*.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial stdio -display none