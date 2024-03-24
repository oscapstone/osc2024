SRC_DIR = src
OBJ_DIR = obj

ARMGNU = aarch64-linux-gnu-
CC = $(ARMGNU)gcc
LD = $(ARMGNU)ld
OBJCPY = $(ARMGNU)objcopy

LINKER = $(SRC_DIR)/linker.ld
START = $(SRC_DIR)/boot.s
START_OBJ = $(OBJ_DIR)/boot.o
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRC)))

FLAG = -Wall -nostdlib -I include -c

.PHONY: all

$(START_OBJ): $(START)
	$(CC) $(FLAG) $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(FLAG) $< -o $@

all: dir kernel8.img

dir:
	mkdir -p $(OBJ_DIR)

kernel8.img: $(OBJ) $(START_OBJ)
	$(LD) $(START_OBJ) $(OBJ) -T $(LINKER) -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf kernel8.img

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

asm:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -S -s

clean:
	rm -rf $(OBJ_DIR) kernel8.*
