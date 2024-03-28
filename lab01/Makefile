SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
ASMS = $(wildcard *.S)
AOBJS = $(ASMS:.S=.o)
CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -fno-stack-protector -g

all: clean kernel8.img

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

kernel8.img: $(AOBJS) $(OBJS)
	aarch64-linux-gnu-ld -nostdlib $(AOBJS) $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf kernel8.img *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -s -S
