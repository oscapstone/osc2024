CC=aarch64-linux-gnu-gcc-10
CFLAGS=-Wall $(INCLUDE_FLAGS) -ffreestanding -g -ffunction-sections
LD=aarch64-linux-gnu-ld
LD_SCRIPT=linker.ld
OBJCOPY=aarch64-linux-gnu-objcopy

INCLUDE_FLAGS=-I ../include -I ./
SRC_DIR=../src
BOOT_DIR=./

SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
SRC_OBJ_FILES=$(SRC_FILES:.c=.o)

BOOT_FILES=$(wildcard $(BOOT_DIR)/*.c)
BOOT_OBJ_FILES=$(BOOT_FILES:.c=.o)

.PHONY: clean run run-mini-uart run-pty

all: kernel8.img

kernel8.img: kernel8.elf
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

kernel8.elf: start.o $(SRC_OBJ_FILES) $(BOOT_OBJ_FILES)
	$(LD) -T $(LD_SCRIPT) -o kernel8.elf \
		start.o $(SRC_OBJ_FILES) $(BOOT_OBJ_FILES)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

start.o: start.S
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm kernel8.* main.o start.o $(OBJ_FILES)

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

run-mini-uart:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio

run-pty:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial pty
