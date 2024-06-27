ARMGNU = aarch64-linux-gnu

CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -Iinclude -mgeneral-regs-only -g
ASMFLAGS = -Iinclude
QEMUFLAGS = -M raspi3b -display none -serial null -serial pty

BUILD_DIR = build
SRC_DIR = src

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(ASMFLAGS) -c $< -o $@

all: clean kernel8.img

kernel8.img: $(SRC_DIR)/link.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/link.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img *.elf *.o

qemu: clean kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

qemu_asm: clean kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -d in_asm 

