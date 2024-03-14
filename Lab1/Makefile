
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

all: clean kernel

start.o: start.S
	$(CC) $(CFLAGS) -c start.S

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel: start.o $(OBJS)
	$(LD) -T linker.ld -o kernel8.elf start.o $(OBJS)
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -s -S

clean:
	rm kernel8.elf kernel8.img *.o