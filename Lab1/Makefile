ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -nostdinc -Iinclude -mgeneral-regs-only -Wextra -Wpedantic -O2 -ggdb
ASMOPS = -Iinclude -ggdb

BUILD_DIR = build
SRC_DIR = src

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

all : kernel8.img

clean :
	rm -rf $(BUILD_DIR)/* *.img 

run :
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel kernel8.img
	
test :
	qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel kernel8.img -S -s

gdb :
	aarch64-linux-gnu-gdb build/kernel8.elf

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -Map $(BUILD_DIR)/kernel8.map -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

#qemu-system-aarch64 -M raspi3b -kernel kernel8.img -nographic -serial null -chardev stdio,id=uart1 -serial chardev:uart1 -monitor none
	