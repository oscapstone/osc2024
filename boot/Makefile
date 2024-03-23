CC=aarch64-linux-gnu-gcc-10
CFLAGS=-Wall -I $(INCLUDE_DIR) -ffreestanding -g -ffunction-sections
LD=aarch64-linux-gnu-ld
LD_SCRIPT=linker.ld
OBJCOPY=aarch64-linux-gnu-objcopy

INCLUDE_DIR=../include
SRC_DIR=../src

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(SRC_FILES:.c=.o)

.PHONY: clean run run-mini-uart

all: kernel8.img

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

kernel8.elf: start.o main.o $(OBJ_FILES)
	$(LD) -T $(LD_SCRIPT) -o kernel8.elf \
		start.o main.o $(OBJ_FILES)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

main.o: main.c
	$(CC) $(CFLAGS) -o $@ -c $<

start.o: start.S
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm kernel8.* $(SRC_DIR)/start.o $(OBJ_FILES)

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

run-mini-uart:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

run-pty:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial pty
