ARMGNU ?= aarch64-linux-gnu
# cross compiler prefix

COPS = -Wall -g -nostdlib -nostartfiles -ffreestanding -Iinclude
# flag for c compiler
# -Wall 		 : open all warning
# -g			 : enable gdb
# -nostdlib 	 : disable C standard lib, because it use OS
# -ffreestanding : tell compiler not assume standard functions have their usual definition
# -linclude      : add include dir to search header file
# -mgeneral-regs-only : only use general-purpose registers 

ASMOPS = -Iinclude
# flag for asm assembler

BUILD_DIR = build
SRC_DIR = src

all : kernel8.img
# target is kernel8.img

clean :
	rm -rf $(BUILD_DIR) *.img *.elf

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@
# compile c to object code
# mkdir create dir with -p option to prevent error message if dir already exists
# $(@D) extract target file dir, eg $(BUILD_DIR)/%_c.o --> $(BUILD_DIR)

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@
## compile asm to object code

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
# for every file in C_FILES, convert from $(SRC_DIR)/%.c to $(BUILD_DIR)/%_c.o

OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)


DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy kernel8.elf -O binary kernel8.img
# aarch64-linux-gnu-ld link obj_files with linker script -T linkder.ld, create -o kernel8.elf
# aarch64-linux-gnu-objcopy convert from elf (execute by kernel) to image


# Script section ----------------------------------------------------------------------------

SESSION="QEMU"
QEMU_CMD="qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -display none -serial null -serial stdio -S -s"
GDB_CMD="gdb-multiarch"

.PHONY: run
run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -display none -serial null -serial stdio -s

.PHONY: debug
debug:
	make close || echo "closed."
	tmux new-session -d -s $(SESSION)
	tmux rename-window -t $(SESSION):0 'Main'
	tmux split-window -v -p 20 -t $(SESSION):Main
	tmux send-keys -t $(SESSION):Main $(QEMU_CMD) C-m
	tmux select-pane -t $(SESSION):Main -U
	tmux send-keys -t $(SESSION):Main $(GDB_CMD) C-m
	tmux send-keys -t $(SESSION):Main "file kernel8.elf" C-m
	tmux send-keys -t $(SESSION):Main "target remote :1234" C-m
	tmux attach -t $(SESSION)

.PHONY: dump
dump: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

.PHONY: close
close:
	tmux kill-session -t $(SESSION)

