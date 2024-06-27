TARGET = kernel8

ARMGNU ?= aarch64-linux-gnu

CFLAGS = -MMD -Wall -nostdlib -nostartfiles -ffreestanding -mgeneral-regs-only -Iinclude -Wno-pointer-to-int-cast -g
ASMOPS = -MMD -Iinclude

OUT_DIR = out
BUILD_DIR = build
SRC_DIR = src

# all: clean $(TARGET).img
all: clean ramdisk $(TARGET).img

debug: CCFLAGS += -DDEBUG -g
# debug: clean $(TARGET).img
debug: clean ramdisk $(TARGET).img


clean:
	rm -rf $(BUILD_DIR) $(OUT_DIR) *.img

remake: clean all


$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@	

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -c $< -o $@

C_FILES   =  $(wildcard $(SRC_DIR)/*.c)
ASM_FILES =  $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES =  $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

SD_DIR = ../sd

ramdisk:
	mkdir -p $(BUILD_DIR)
# 	$(ARMGNU)-ld -r -b binary -o $(BUILD_DIR)/rd.o $(SD_DIR)/initramfs.cpio

# OBJ_FILES += $(BUILD_DIR)/rd.o

$(TARGET).img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	mkdir -p $(OUT_DIR)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/$(TARGET).elf $(OBJ_FILES) 
	$(ARMGNU)-objcopy $(BUILD_DIR)/$(TARGET).elf -O binary $(OUT_DIR)/$(TARGET).img

run: $(OUT_DIR)/$(TARGET).img
	qemu-system-aarch64 -M raspi3b -kernel $(OUT_DIR)/$(TARGET).img -serial null -serial stdio -initrd $(SD_DIR)/initramfs.cpio -dtb $(SD_DIR)/bcm2710-rpi-3-b-plus.dtb

gdb: $(OUT_DIR)/$(TARGET).img
	qemu-system-aarch64 -M raspi3b -kernel $(OUT_DIR)/$(TARGET).img -serial null -serial stdio -initrd $(SD_DIR)/initramfs.cpio -dtb $(SD_DIR)/bcm2710-rpi-3-b-plus.dtb -S -s