CC = clang
C_FLAGS = -nostdinc -nostdlib -ffreestanding -mgeneral-regs-only -Iinclude

SRC_DIR = src
SRCS 	= $(wildcard $(SRC_DIR)/*.c)
OBJ_DIR = obj
OBJS 	= $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS)) $(OBJ_DIR)/boot.o

CPIO_DIR = ../rootfs

all: kernel8.img

kernel8.img: kernel8.elf
	llvm-objcopy --output-target=aarch64-rpi3-elf -O binary kernel8.elf kernel8.img

kernel8.elf: $(OBJS)
	ld.lld -nostdlib -m aarch64elf -T src/linker.ld -o kernel8.elf $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(C_FLAGS) -mcpu=cortex-a53 --target=aarch64-rpi3-elf -c $< -o $@

$(OBJ_DIR)/boot.o: $(SRC_DIR)/boot.s
	$(CC) -mcpu=cortex-a53 --target=aarch64-rpi3-elf -c $< -o $@

.PHONY: run debug clean cpio
run: kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd ../initramfs.cpio -dtb ../bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	rm -rf $(OBJ_DIR) *.img *.elf

cpio:
	cd $(CPIO_DIR) ; find . | cpio -o -H newc > ../initramfs.cpio
