
GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

CFLAGS	= -Wall -Wextra -Wpedantic -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

SRC_FILES = $(wildcard *.c)

OBJ_DIR = bin-int

OBJ_FILES = $(SRC_FILES:.c=.o) start.o

all : bootloader.img

bootloader.img : $(OBJ_FILES)
	$(LD) $^ -T linker.ld -o bootloader.elf
	$(OBJCOPY) -O binary bootloader.elf $@

run : bootloader.img
	# qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none -device loader,file=$<,addr=0x80000,cpu-num=0
	qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none -kernel $< -initrd ../../initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
	
debugRun : bootloader.img
	# qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none -device loader,file=$<,addr=0x80000,cpu-num=0 -S -s
	qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none -kernel $< -initrd ../../initramfs.cpio -S -s -dtb bcm2710-rpi-3-b-plus.dtb

gdb-run :
	gdb-multiarch -ex 'set architecture aarch64' -ex 'file bootloader.elf' -ex 'target remote localhost:1234'

clean:
	rm $(OBJ_FILES) bootloader.img

%.o : %.S
	$(GCC) $(CFLAGS) -c $< -o $@

%.o : %.c
	$(GCC) $(CFLAGS) -c $< -o $@

