CC 			= clang
LD 			= clang -fuse-ld=lld
OBJCOPY		= llvm-objcopy

TARGET 		= aarch64-unknown-none-elf
CFLAGS 		= -Wall -Wextra -Wshadow \
			  -ffreestanding -mgeneral-regs-only \
			  -mcpu=cortex-a53 --target=$(TARGET) \
			  -nostdinc -nostdlib -Os

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

.PHONY: all build clean run

all: build run

build: kernel8.img

start.o: start.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.elf: start.o $(OBJS)
	$(LD) -T linker.ld $(CFLAGS) $^ -o $@

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary $< $@

clean:
	$(RM) kernel8.img kernel8.elf *.o

run: build
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm,cpu -smp cpus=4
