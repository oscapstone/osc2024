GNUARM ?= aarch64-linux-gnu

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -g

SRC_C = $(wildcard *.c) # find all .c file
SRC_S = $(wildcard *.S) # find all .S file
OBJS = $(SRC_C:.c=.o) # replace all .c in SRC_C with .o by using %.o:%.c
OBJS += $(SRC_S:.S=.o) # replace all .S in SRC_S with .o by using %.o:%.S

all: bootloader.img

%.o: %.S
	$(GNUARM)-gcc $(CFLAGS) -c $< -o $@ 

%.o: %.c
	$(GNUARM)-gcc $(CFLAGS) -c $< -o $@

bootloader.img: linker.ld $(OBJS)
	$(GNUARM)-ld -T linker.ld -o bootloader.elf $(OBJS)
	$(GNUARM)-objcopy -O binary bootloader.elf bootloader.img

qemu:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial stdio -display none

boot:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial pty -display none -dtb bcm2710-rpi-3-b-plus.dtb -initrd ../initramfs.cpio

clean:
	rm *.o bootloader.*