ARMGNU ?= aarch64-linux-gnu

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude

BUILD_DIR = build
SRC_DIR = src
#---------------------------------------------------------------------------------------

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(CFLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMFLAGS) -MMD -c $< -o $@

bootloader.img: $(SRC_DIR)/link.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/link.ld -o $(BUILD_DIR)/bootloader.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/bootloader.elf -O binary bootloader.img

all: bootloader.img

clean:
	rm -rf $(BUILD_DIR) *.img

run:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
	
run_pty:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial pty -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -S -s

