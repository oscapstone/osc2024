CROSS_PLATFORM = aarch64-linux-gnu
CC = gcc
KERNEL = kernel8
# -nostdlib Don't use the C standard library. 
# -nostartfiles Don't use standard startup files. Startup files are responsible for setting an initial stack pointer, initializing static data, and jumping to the main entry point. 
# -ffreestanding A freestanding environment is an environment in which the standard library may not exist, and program startup may not necessarily be at main. The option -ffreestanding directs the compiler to not assume that standard functions have their usual definition.
# -mgeneral-regs-only. Use only general-purpose registers. ARM processors also have NEON registers. We don't want the compiler to use them because they add additional complexity 
# -nostdinc Do not search for system include library
CFLAGS =  -Wall -nostdlib -nostartfiles -ffreestanding -nostdinc -Iinclude -mgeneral-regs-only
CFLAGS += -I /usr/lib/gcc-cross/aarch64-linux-gnu/11/include
# -Iinclude Search for header files in the include folder.

KER_SRC = ./src/kernel
KER_HEAD = ./include
COMMON_SRC = ./src/common

KERSRCS = $(wildcard $(KER_SRC)/*.c)
OBJS = $(notdir $(KERSRCS:.c=.o))

ASM = $(wildcard $(KER_SRC)/*.S)
# replace the .S $(ASM) to .o
# then extracr file name without path
ASM_OBJS = $(notdir $(ASM:.S=.o))

all: kernel8.img
# .o files depend on .S files
%.o: $(KER_SRC)/%.S
	$(CROSS_PLATFORM)-$(CC) $(CFLAGS) -c $< -o $@
%.o: $(KER_SRC)/%.c
	$(CROSS_PLATFORM)-$(CC) $(CFLAGS) -c $< -o $@
# kernel8.ing depends on ASM_OBJS and OBJS, recursively compile .S or .c files to .o
kernel8.img: $(ASM_OBJS) $(OBJS) 
	$(CROSS_PLATFORM)-ld -T $(KER_SRC)/linker.ld -o $(KERNEL).elf $(ASM_OBJS) $(OBJS)
	$(CROSS_PLATFORM)-objcopy -O binary $(KERNEL).elf $(KERNEL).img

clean:
	rm *.elf *.img *.o

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio