SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -mcpu=cortex-a53 -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -nostdinc -Iinclude

all: clean kernel8.img

start.o: start.S
	clang --target=aarch64-rpi3-elf $(CFLAGS) -c start.S -o start.o

%.o: %.c
	clang --target=aarch64-rpi3-elf $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	ld.lld -m aarch64elf -nostdlib start.o $(OBJS) -T linker.ld -o kernel8.elf
	llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	-@rm -v kernel8.elf *.o

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -d in_asm
