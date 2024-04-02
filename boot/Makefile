CC=aarch64-linux-gnu-gcc-10
CFLAGS=-Wall $(INCLUDE_FLAGS) -ffreestanding -g -ffunction-sections
LD=aarch64-linux-gnu-ld
LD_SCRIPT=linker.ld
OBJCOPY=aarch64-linux-gnu-objcopy

INCLUDE_FLAGS=-I $(BOOT_DIR) -I $(SRC_DIR)
SRC_DIR=../src
BOOT_DIR=.
DIST_DIR=dist

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
SRC_OBJ_FILES=$(SRC_FILES:.c=.o)

BOOT_FILES=$(wildcard $(BOOT_DIR)/*.c)
BOOT_OBJ_FILES=$(BOOT_FILES:.c=.o)

DTB_FILE=../bcm2710-rpi-3-b-plus.dtb


.PHONY: clean run run-mini-uart run-pty

all: kernel8.img initramfs.cpio

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary \
		$(DIST_DIR)/kernel8.elf \
		$(DIST_DIR)/kernel8.img

kernel8.elf: start.o $(SRC_OBJ_FILES) $(BOOT_OBJ_FILES)
	mkdir -p $(DIST_DIR)
	$(LD) -T $(LD_SCRIPT) -o $(DIST_DIR)/kernel8.elf \
		start.o $(SRC_OBJ_FILES) $(BOOT_OBJ_FILES)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

start.o: start.S
	$(CC) $(CFLAGS) -o $@ -c $<

initramfs.cpio:
	cd ../rootfs && find . | cpio -o -H newc > ../initramfs.cpio

clean:
	rm -f start.o $(SRC_OBJ_FILES) $(BOOT_OBJ_FILES)
	rm -rf $(DIST_DIR)
	rm -rf ../initramfs.cpio

run:
	qemu-system-aarch64 -M raspi3b -kernel $(DIST_DIR)/kernel8.img -display none -d in_asm

run-mini-uart:
	qemu-system-aarch64 -M raspi3b -kernel $(DIST_DIR)/kernel8.img -display none -serial null -serial stdio

run-pty:
	qemu-system-aarch64 -M raspi3b -dtb ../bcm2710-rpi-3-b-plus.dtb -initrd ../initramfs.cpio -kernel $(DIST_DIR)/kernel8.img -display none -serial null -serial pty
