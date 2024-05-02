CPIO = ../initramfs.cpio
DTB = ../bcm2710-rpi-3-b-plus.dtb

BUILD_DIR = ../build
LIB_DIR = ../lib

CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(LIB_SRCS:$(LIB_DIR)/%.c=$(BUILD_DIR)/lib/%.o)

ASM_SRCS = $(wildcard *.S)
ASM_OBJS = $(ASM_SRCS:%.S=$(BUILD_DIR)/%.o)

LIB_INCLUDE = -I $(LIB_DIR)/include

INCLUDES = -I ./include \
	$(LIB_INCLUDE)

all: clean kernel

$(BUILD_DIR)/%.o: %.S
	$(shell [ ! -d $(@D) ] && mkdir -p $(@D))
	$(CC) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	$(shell [ ! -d $(@D) ] && mkdir -p $(@D))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c
	$(shell [ ! -d $(@D) ] && mkdir -p $(@D))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

kernel: $(OBJS) $(LIB_OBJS) $(ASM_OBJS)
	$(LD) -T linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJS) $(LIB_OBJS) $(ASM_OBJS)
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel8.elf kernel8.img

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd $(CPIO) -display none -serial null -serial stdio -dtb $(DTB)

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd $(CPIO) -display none -serial null -serial stdio -s -S

clean:
	rm $(BUILD_DIR)/lib/*
	rmdir $(BUILD_DIR)/lib
	rm $(BUILD_DIR)/*
	rmdir $(BUILD_DIR)
