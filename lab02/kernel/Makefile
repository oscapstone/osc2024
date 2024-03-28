SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -fno-stack-protector -g
# Can't use mailbox  version => -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

all: clean kernel8.img size

size: kernel8.img
	../make_size.sh ./kernel8.img


start.o: start.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c $^ -o $@

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	aarch64-linux-gnu-ld -nostdlib start.o $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.elf kernel8.img kernel8.bin *.o >/dev/null 2>/dev/null || true
	clear


cpio: clean all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -serial null -serial stdio -display none


run: clean all run
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -serial null -serial stdio -display none -dtb bcm2710-rpi-3-b-plus.dtb
	
debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -display none -serial null -serial stdio -s -S