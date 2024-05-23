# ARMGNU ?= aarch64-linux-gnu

CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy
GDB := gdb-multiarch
QEMU := qemu-system-aarch64

CFLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMFLAGS = -Iinclude

# echo "DEBUG MODE"
DEBUG = 1
# Set the TARGET_PLATFORM and _DEBUG macro to be passed to the GCC definition later.
ifdef DEBUG
$(info DEBUG MODE)
CFLAGS+=-D_DEBUG=$(DEBUG)
endif

NAME = kernel8
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
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(CC) $(ASMFLAGS) -MMD -c $< -o $@

$(NAME).img: $(SRC_DIR)/link.ld $(OBJ_FILES)
	$(LD) -T $(SRC_DIR)/link.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(OBJCOPY) $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

all: 
	$(NAME).img

clean:
	rm -rf $(BUILD_DIR) *.img

run:
	rm -rf $(BUILD_DIR) *.img
	make
	qemu-system-aarch64 -M raspi3b -display none -kernel kernel8.img -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb

debug:
	qemu-system-aarch64 -M raspi3b -display none -kernel kernel8.img -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -S -s

