ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles -fno-stack-protector -g -Iinclude
ASMOPS = 

BUILD_DIR = build
SRC_DIR = src


all: clean bootloader.img

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *.img

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.s
	$(ARMGNU)-gcc $(ASMOPS) -c $< -o $@


C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

# DEP_FILES = $(OBJ_FILES:%.o=%.d)
# -include $(DEP_FILES)

bootloader.img: $(OBJ_FILES) $(SRC_DIR)/linker.ld
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/bootloader.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/bootloader.elf -O binary bootloader.img

.PHONY: run debug test
run: bootloader.img
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial pty \
									-initrd ../initramfs.cpio \
									-dtb ../bcm2710-rpi-3-b-plus.dtb

test: bootloader.img
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial stdio

dump: bootloader.img
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -d in_asm

debug: all
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial stdio -S -s
