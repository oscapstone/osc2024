GCC = "aarch64-linux-gnu-gcc"
LD = "aarch64-linux-gnu-ld"
OBJCPY = "aarch64-linux-gnu-objcopy"
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

all: clean kernel8.img

start.o: start.S
	$(GCC) $(CFLAGS) -c start.S -o start.o

%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@ -g

kernel8.img: start.o $(OBJS)
	$(LD) --nostdlib start.o $(OBJS) -T link.ld -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none
