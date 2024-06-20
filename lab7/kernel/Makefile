CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy
GDB := gdb-multiarch
QEMU := qemu-system-aarch64
KILL_QEMU := killall $(QEMU)

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only $(cops)
ASMOPS = -Iinclude 
QOPS = -M raspi3b -kernel $(NAME).img -dtb ../bcm2710-rpi-3-b-plus.dtb -initrd ../initramfs.cpio  
GDBOPS = --init-eval-command="file $(BUILD_DIR)/$(NAME).elf" --init-eval-command="target remote :1234"

RED=\033[0;31m
GREEN=\033[0;32m
YELLOW=\033[0;33m
BLUE=\033[0;34m
PURPLE=\033[0;35m
CYAN=\033[0;36m
RESET=\033[0m

# Set the TARGET_PLATFORM and _DEBUG macro to be passed to the GCC definition later.
ifdef DEBUG
COPS+=-D_DEBUG=$(DEBUG)
endif

ifneq (,$(findstring pi,$(MAKECMDGOALS)))
COPS+=-DRPI
FINISH_STR = "\r\n-------------------------------------------------------------\r\n The $(CYAN)kernel8.img$(RESET) intended for the $(BLUE)Raspberry Pi 3B+$(RESET) is ready.\r\n-------------------------------------------------------------"
else # Default to QEMU 
COPS += -DQEMU
FINISH_STR = "\r\n-------------------------------------------------\r\n The $(CYAN)kernel8.img$(RESET) intended for the $(BLUE)QEMU$(RESET) is ready.\r\n-------------------------------------------------"
endif

NAME = kernel8
BUILD_DIR = build
SRC_DIR = src
LK_SCRIPT = linker.ld
SESSION_NAME := my-os

.PHONY: all
all : $(NAME).img

.PHONY: clean
clean :
	rm -rf $(BUILD_DIR) *.img 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(CC) $(ASMOPS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

$(NAME).img: $(SRC_DIR)/$(LK_SCRIPT) $(OBJ_FILES)
	$(LD) -T $(SRC_DIR)/$(LK_SCRIPT) -o $(BUILD_DIR)/$(NAME).elf  $(OBJ_FILES)
	$(OBJCOPY) $(BUILD_DIR)/$(NAME).elf -O binary $(NAME).img
	@echo -e $(FINISH_STR)
	
#This target compiles and runs the kernel.img of the QEMU platform by default.
# You can use "make pi run" to run this image in QEMU.
.PHONY: run
run: clean  $(NAME).img
	$(QEMU) $(QOPS) -serial null -serial stdio

# This target compiles kernel.img to run on a QEMU.
.PHONY: qemu
qemu: clean  $(NAME).img
	
# This target compiles kernel.img to run on a Raspberry 3B.
.PHONY: pi
pi: clean $(NAME).img

# This target will run kernel in QEMU with GDB attached in a tmux session.
.PHONY: debug
debug: clean $(NAME).img
	tmux new-session -d -s $(SESSION_NAME) '$(QEMU) $(QOPS) -S -s  -serial null -serial stdio' \; split-window -h '$(GDB) $(GDBOPS)' \; attach -t $(SESSION_NAME)

# This target will kill the QEMU session.
.PHONY: kill
kill: 
	$(KILL_QEMU)
