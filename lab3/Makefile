ARMGNU ?= aarch64-linux-gnu

CFLAGS = -Iinclude -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -Wall -g
ASMFLAGS = -Iinclude
QEMUFLAGS = -M raspi3b -display none -serial null -serial stdio -d int

SRC_DIR = src
BUILD_DIR = build

all: clean kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(CFLAGS)   -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	$(ARMGNU)-gcc $(ASMFLAGS) -c $< -o $@

SRC_FILES  = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES  = $(wildcard *.S)
OBJ_FILES  = $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
OBJ_FILES += $(ASM_FILES:%.S=$(BUILD_DIR)/%.o)

kernel8.img: linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy -O binary  $(BUILD_DIR)/kernel8.elf kernel8.img

qemu: all kernel8.img initramfs.cpio bcm2710-rpi-3-b-plus.dtb
	clear
	qemu-system-aarch64 $(QEMUFLAGS) -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug: all kernel8.img initramfs.cpio bcm2710-rpi-3-b-plus.dtb
	qemu-system-aarch64 $(QEMUFLAGS) -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -S -s