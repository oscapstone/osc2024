# all:
# 	aarch64-linux-gnu-gcc -c a.S
# 	aarch64-linux-gnu-ld -T linker.ld -o kernel8.elf a.o
# 	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img

# clean:
# 	rm -f kernel8.img kernel8.elf *.o 

# check:
# 	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -d in_asm

# flash:
# 	sudo dd if=kernel8.img of=/dev/sdb

CC = aarch64-linux-gnu

CFLAGS = -Wall -g -nostdlib -nostartfiles -ffreestanding -Iinclude

KER_SRC = src
OBJ_DIR = build
ASMSOURCES = $(wildcard $(KER_SRC)/*.S)
OBJECTS = $(patsubst $(KER_SRC)/%.S, $(OBJ_DIR)/%.o, $(ASMSOURCES))

IMG_NAME = kernel8.img

all: $(OBJECTS)
	$(CC)-ld -T $(KER_SRC)/linker.ld -o $(OBJ_DIR)/kernel8.elf $(OBJECTS)
	$(CC)-objcopy -O binary $(OBJ_DIR)/kernel8.elf $(OBJ_DIR)/$(IMG_NAME)

$(OBJ_DIR)/%.o: $(KER_SRC)/%.S
	@mkdir -p $(OBJ_DIR)
	$(CC)-gcc -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
