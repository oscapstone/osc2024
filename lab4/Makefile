# ARMGNU ?= aarch64-linux-gnu
ARMGNU ?= aarch64-none-elf

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude

BUILD_DIR = build
SRC_DIR = .
KERNEL_NAME = kernel8
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

$(KERNEL_NAME).img: $(SRC_DIR)/link.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/link.ld -o $(BUILD_DIR)/$(KERNEL_NAME).elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/$(KERNEL_NAME).elf -O binary $(KERNEL_NAME).img

all: $(KERNEL_NAME).img

clean:
	rm -rf $(BUILD_DIR) $(KERNEL_NAME).img

run:
	qemu-system-aarch64 -M raspi3b -kernel $(KERNEL_NAME).img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -kernel $(KERNEL_NAME).img -display none -S -s

gdb:
	$(ARMGNU)-gdb -x debug.gdb