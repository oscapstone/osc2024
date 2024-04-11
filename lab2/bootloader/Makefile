SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -Wextra -Wpedantic -Werror -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles 
CFLAGS += -I /usr/aarch64-linux-gnu/include

all: boot_kernel.img

start.o: start.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

boot_kernel.img: start.o $(OBJS)
	aarch64-linux-gnu-ld start.o $(OBJS) -T linker.ld -o boot_kernel.elf
	aarch64-linux-gnu-objcopy -O binary boot_kernel.elf boot_kernel.img

clean:
	rm boot_kernel.elf *.o *~>/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none  -kernel boot_kernel.img