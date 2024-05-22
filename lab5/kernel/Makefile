OS := $(shell uname)

ifeq ($(OS), Linux)
	ARMGNU  = aarch64-linux-gnu
	MACHINE = raspi3b
endif

ifeq ($(OS), Darwin)
	ARMGNU  = aarch64-unknown-linux-gnu
	MACHINE = raspi3b
endif

CC     = $(ARMGNU)-gcc
LK     = $(ARMGNU)-ld
OBJCPY = $(ARMGNU)-objcopy
QEMU   = qemu-system-aarch64

A_SRCS = $(wildcard lib/*.S)
C_SRCS = $(wildcard lib/*.c)
SRC_DIR = lib
OBJ_DIR = build
OBJS = $(A_SRCS:$(SRC_DIR)/%.S=$(OBJ_DIR)/%_s.o) $(C_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%_c.o)
INCLUDE = include
CFLAGS = -ggdb -Wno-implicit -Wno-int-conversion -ffreestanding -nostdlib -nostartfiles -I$(INCLUDE) 
CPIO_FILE = ../initramfs.cpio
DTB_FILE = ../bcm2710-rpi-3-b-plus.dtb

IMAGE = kernel8

all: $(IMAGE).img 

$(IMAGE).img: $(IMAGE).elf
	$(OBJCPY) -O binary $(IMAGE).elf $(IMAGE).img

$(IMAGE).elf: $(OBJS)
	$(LK) $(OBJS) -T linker.ld -o $(IMAGE).elf

$(OBJ_DIR)/%_s.o: lib/%.S
	@if [ ! -d "$(OBJ_DIR)" ]; then mkdir $(OBJ_DIR); fi
	$(CC) -c $< $(CFLAGS) -o $@

$(OBJ_DIR)/%_c.o: lib/%.c
	@if [ ! -d "$(OBJ_DIR)" ]; then mkdir $(OBJ_DIR); fi
	$(CC) -c $< $(CFLAGS) -o $@

run: $(IMAGE).img 
	$(QEMU) -machine $(MACHINE) -kernel $(IMAGE).img -initrd $(CPIO_FILE) -dtb $(DTB_FILE) -display none -serial null -serial stdio

run_display: $(IMAGE).img
	$(QEMU) -machine $(MACHINE) -kernel $(IMAGE).img -initrd $(CPIO_FILE) -dtb $(DTB_FILE) -serial null -serial stdio

dbg: $(IMAGE).img 
	$(QEMU) -machine $(MACHINE) -kernel $(IMAGE).img -initrd $(CPIO_FILE) -dtb $(DTB_FILE) -display none -serial null -serial stdio -s -S

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) *.img *.elf */*.elf