ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -ffreestanding -nostartfiles -nostdlib -g -Iinclude 
ASMOPS = -Iinclude

BUILD_DIR = build
SRC_DIR = src

all: clean makedir kernel8.img


ifdef DEBUG
COPS += -DDEBUG
endif
# DEBUG: COPS += -DDEBUG
# DEBUG: all

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *.img

makedir:
	mkdir -p $(BUILD_DIR)
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@


C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(OBJ_FILES) $(SRC_DIR)/linker.ld
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

.PHONY: run debug QEMU
run: clean makedir kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -initrd ../initramfs.cpio

dump: kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

gdb: clean makedir kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -S -s -initrd ../initramfs.cpio

QEMU: COPS += -DQEMU
QEMU: run