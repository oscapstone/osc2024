SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostdinc

all: clean initram kernel8.img

start.o: start.S
	aarch64-elf-gcc $(CFLAGS) -c start.S -o start.o

%.o: %.c
	aarch64-elf-gcc $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-elf-ld -nostdlib -nostartfiles start.o $(OBJS) -T link.ld -o kernel8.elf
	aarch64-elf-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm -f kernel8.elf *.o initramfs.cpio

initram:
	echo "Creating initramfs.cpio..."
	cd rootfs && find . | cpio -o -H newc > ../initramfs.cpio

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial stdio -initrd initramfs.cpio
