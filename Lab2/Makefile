CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy
CFLAGS := -Wall -Wextra -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles
CFLAGS += -IMain -IUART/header -IShell/header -IUtils/header -ICPIO/header -IDevicetree/header -IAllocator/header
CFLAGS += -I/usr/lib/gcc-cross/aarch64-linux-gnu/11/include

K8 := kernel8
KERNEL_LINKER := Main/linker.ld
KERNEL_SRC_S := $(wildcard Main/*.s)
KERNEL_SRC_C := $(wildcard Main/*.c UART/*.c Shell/*.c Utils/*.c CPIO/*.c Devicetree/*.c Allocator/*.c)
KERNEL_OBJS := $(KERNEL_SRC_C:.c=.o) $(KERNEL_SRC_S:.s=.o) Main/main.o

BL := bootloader
BOOTLOADER_LINKER := Bootloader/linker.ld
BOOTLOADER_SRC_S := $(shell find Bootloader/ -name '*.s')
BOOTLOADER_SRC_C := $(wildcard Bootloader/*.c UART/*.c Utils/*.c)
BOOTLOADER_OBJS := $(BOOTLOADER_SRC_C:.c=.o) $(BOOTLOADER_SRC_S:.s=.o)

all: $(K8).img $(BL).img

$(K8).img: $(KERNEL_OBJS)
	$(LD) -T $(KERNEL_LINKER) -o $(K8).elf $^
	$(OBJCOPY) -O binary $(K8).elf $@

$(BL).img: $(BOOTLOADER_OBJS)
	$(LD) -T $(BOOTLOADER_LINKER) -o $(BL).elf $^
	$(OBJCOPY) -O binary $(BL).elf $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

sd: $(BL).img
	cp $(K8).img /media/ben/4DFF-0A36
	sync

qemu: 
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none  -initrd Other/Archive/initramfs.cpio -dtb Other/Archive/bcm2710-rpi-3-b-plus.dtb -kernel $(K8).img

bl:
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -serial pty -display none -kernel $(BL).img -S -s

rpi3:
	screen /dev/ttyUSB0 115200

py:	
	python3 Bootloader/send.py

clean:
	rm -f $(wildcard */*.o) $(wildcard *.o) $(K8).elf $(K8).img $(BL).elf $(BL).img

