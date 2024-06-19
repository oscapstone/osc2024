ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only -ggdb -Wunused-variable
ASMOPS = -Iinclude -ggdb

BUILD_DIR = build
SRC_DIR = src

all : kernel8.img

clean :
	rm -rf $(BUILD_DIR) *.img 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

test: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -initrd things_to_sd_card/initramfs.cpio -dtb things_to_sd_card/bcm2710-rpi-3-b-plus.dtb

debug: 
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio -initrd things_to_sd_card/initramfs.cpio -dtb things_to_sd_card/bcm2710-rpi-3-b-plus.dtb -s -S

send_pre:
	source ../../pyenv_osc2024/bin/activate   

send:
	python3 send_img_uart.py -t /dev/tty.usbserial-0001 -i ./kernel8.img

run:
	minicom -D /dev/tty.usbserial-0001
