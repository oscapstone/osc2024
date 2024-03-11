CC = aarch64-linux-gnu
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -Iinclude

SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build

OBJECTS = $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/*.S))
OBJECTS += $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

ELF_NAME = kernel8.elf
IMG_NAME = kernel8.img

all: $(OBJECTS) $(HEADERS)
	$(CC)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/$(ELF_NAME) $(OBJECTS)
	$(CC)-objcopy -O binary $(BUILD_DIR)/$(ELF_NAME) $(BUILD_DIR)/$(IMG_NAME)


$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(BUILD_DIR)
	$(CC)-gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

dump:
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/$(IMG_NAME) -display none -d in_asm

run:
	qemu-system-aarch64 -M raspi3b -kernel $(BUILD_DIR)/$(IMG_NAME) -display none -serial null -serial stdio -s
