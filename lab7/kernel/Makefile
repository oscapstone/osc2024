ARMGNU ?= aarch64-elf

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only -g
ASMFLAGS = -Iinclude

BUILD_DIR = build
SRC_DIR = src

GDB := gdb-multiarch
QEMU := qemu-system-aarch64
QOPS = -M raspi3b -kernel kernel8.img -dtb bcm2710-rpi-3-b-plus.dtb -initrd initramfs.cpio  -display none
GDBOPS = --init-eval-command="file $(BUILD_DIR)/kernel8.elf" --init-eval-command="target remote :1234"
SESSION_NAME := my-os
#---------------------------------------------------------------------------------------

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(ARMGNU)-gcc $(CFLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMFLAGS) -MMD -c $< -o $@

kernel8.img: $(SRC_DIR)/link.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/link.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

.PHONY: all clean run debug

all: clean kernel8.img

clean:
	rm -rf $(BUILD_DIR) *.img

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug:
	tmux new-session -d -s $(SESSION_NAME) '$(QEMU) $(QOPS) -S -s  -serial null -serial stdio' \; split-window -h '$(GDB) $(GDBOPS)' \; attach -t $(SESSION_NAME)
#qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -S -s