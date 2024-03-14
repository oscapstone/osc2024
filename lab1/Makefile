C_SRCS = $(wildcard *.c)
C_OBJS = $(C_SRCS:.c=.o)

ASM_SRCS = $(wildcard *.S)
ASM_OBJS = $(ASM_SRCS:.S=.o)

CFLAGS = -Wall -Wextra -Wpedantic -Werror -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

all: kernel8.img

%.o: %.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

kernel8.img: $(ASM_OBJS) $(C_OBJS)
	aarch64-linux-gnu-ld $(ASM_OBJS) $(C_OBJS) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf kernel8.img *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel kernel8.img