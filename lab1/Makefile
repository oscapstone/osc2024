### variables
SRC_DIR = src
OBJ_DIR = obj
SRC_FILES= $(wildcard $(SRC_DIR)/*.c)

# for files in SRC_FILES, turn them from src/*.c to obj/*.o
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CFLAGS = -g -Wall -mcpu=cortex-a53 -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc -Iinclude

### dependencies
all: clean kernel8.img

start.o: start.S
	clang --target=aarch64-rpi3-elf $(CFLAGS) -c start.S -o start.o

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	clang --target=aarch64-rpi3-elf $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJ_FILES)
	ld.lld -m aarch64elf -nostdlib start.o $(OBJ_FILES) -T linker.ld -o kernel8.elf
	llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	-@rm -v kernel8.*
	-@rm -rv obj

run: kernel8.img
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -S -s -display none
