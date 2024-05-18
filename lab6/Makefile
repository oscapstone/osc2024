ARMGNU ?= aarch64-linux-gnu

CFLAGS = -Iinclude -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -Wall -g
ASMFLAGS = -Iinclude
QEMUFLAGS = -M raspi3b -display none -serial null -serial stdio

SRC_DIR = src
BUILD_DIR = build

all: clean kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img

build:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/_%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(ARMGNU)-gcc $(ASMFLAGS) -c $< -o $@

SRC_FILES  = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES  = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES  = $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/_%.o)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy -O binary $(BUILD_DIR)/kernel8.elf kernel8.img

qemu: all kernel8.img initramfs.cpio bcm2710-rpi-3-b-plus.dtb
	clear & qemu-system-aarch64 $(QEMUFLAGS) -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug: all kernel8.img initramfs.cpio bcm2710-rpi-3-b-plus.dtb
	qemu-system-aarch64 $(QEMUFLAGS) -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -S -s -d int