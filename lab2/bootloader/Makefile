SRC_DIR = src
OBJ_DIR = obj

ARMGNU = aarch64-linux-gnu-
CC = $(ARMGNU)gcc
LD = $(ARMGNU)ld
OBJCPY = $(ARMGNU)objcopy

LINKER = $(SRC_DIR)/linker.ld
START = $(SRC_DIR)/boot.s
START_OBJ = $(OBJ_DIR)/start.o
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRC)))

FLAG = -Wall -nostdlib -I include -c -g
.PHONY: all

$(START_OBJ): $(START)
	$(CC) $(FLAG) $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(FLAG) $< -o $@

all: clean dir kernel8.img

dir:
	mkdir -p $(OBJ_DIR)

kernel8.img: $(OBJ) $(START_OBJ)
	$(LD) $(START_OBJ) $(OBJ) -T $(LINKER) -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf bootloader.img

tty: all
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial pty

run: all
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial stdio

asm:
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -d in_asm

debug: all
	qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -S -s

SESSION="QEMU"
QEMU_CMD="qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial stdio -S -s"
GDB_CMD="gdb-multiarch"
debug_kkj_god:  
	make close || echo "closed."
	tmux new-session -d -s $(SESSION)
	tmux rename-window -t $(SESSION):0 'Main'
	tmux split-window -v -p 20 -t $(SESSION):Main
	tmux send-keys -t $(SESSION):Main $(QEMU_CMD) C-m
	tmux select-pane -t $(SESSION):Main -U
	tmux send-keys -t $(SESSION):Main $(GDB_CMD) C-m
	tmux send-keys -t $(SESSION):Main "file bootloader.elf" C-m
	tmux send-keys -t $(SESSION):Main "target remote :1234" C-m
	tmux attach -t $(SESSION)

close_kkj_god:
	tmux kill-session -t $(SESSION)

clean:
	rm -rf $(OBJ_DIR) bootloader.img kernel8.elf
