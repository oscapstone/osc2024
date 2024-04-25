CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g 
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

ASMS = $(wildcard *.S)
ASM_OBJS = $(ASMS:.S=.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

all:: clean_img flash clean

%.o: %.S
	aarch64-linux-gnu-gcc $(CFLAGS)  -c $< -o $@


%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS)  -c $< -o $@

flash: $(ASM_OBJS) $(OBJS)
	aarch64-linux-gnu-ld $(ASM_OBJS) $(OBJS) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf shell.img
clean:
	rm -f $(ASM_OBJS) $(OBJS) $(DEP_FILES)
	
clean_img:
	rm -f kernel8.elf
	rm -f shell.img
test:
	qemu-system-aarch64 -machine raspi3b -kernel shell.img  -display none -serial null -serial stdio -initrd ../rootfs/initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb
screen:
	sudo screen /dev/ttyUSB0 115200