CC := aarch64-linux-gnu-gcc
LD := aarch64-linux-gnu-ld
OBJCOPY := aarch64-linux-gnu-objcopy
GDB := gdb-multiarch
QEMU := qemu-system-aarch64
KILL_QEMU := killall $(QEMU)


COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 
QOPS = -M raspi3b -kernel $(NAME).img -dtb ../bcm2710-rpi-3-b-plus.dtb -initrd ../initramfs.cpio  -display none
GDBOPS = --init-eval-command="file $(BUILD_DIR)/$(NAME).elf" --init-eval-command="target remote :1234"

NAME = bootloader
BUILD_DIR = build
SRC_DIR = src
LK_SCRIPT = linker.ld
SESSION_NAME := my-os

all : $(NAME).img

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


run: $(NAME).img
	$(QEMU) $(QOPS) -serial null -serial stdio

run_pty: $(NAME).img
	$(QEMU) $(QOPS) -serial null -serial pty > qemu_pty_output.tmp &
	sleep 0.1
	screen `cat qemu_pty_output.tmp | grep -o '/dev/pts/[0-9]*'` # press ctrl+a then d to detach from screen
	$(KILL_QEMU)
	rm qemu_pty_output.tmp
	
debug: $(NAME).img
	tmux new-session -d -s $(SESSION_NAME) '$(QEMU) $(QOPS) -S -s  -serial null -serial stdio' \; split-window -h '$(GDB) $(GDBOPS)' \; attach -t $(SESSION_NAME)

kill: 
	$(KILL_QEMU)