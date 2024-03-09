SRC_DIR = src
BUILD_DIR = build

CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall

RC = rustc
RUSTFLAGS = --crate-type=staticlib --emit=obj --target=aarch64-unknown-linux-gnu -C panic=abort -C opt-level=s -C lto

LINKER = aarch64-linux-gnu-ld
OBJ_CPY = aarch64-linux-gnu-objcopy

QEMU = qemu-system-aarch64
QEMU_FLAGS = -M raspi3b -serial null -serial stdio

TARGET = $(BUILD_DIR)/kernel8.img

dir_guard=@mkdir -p $(@D)

.PHONY: all clean run debug

all: $(TARGET)

$(BUILD_DIR)/start.o: $(SRC_DIR)/start.s
	$(dir_guard)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.rs $(shell find $(SRC_DIR)/ -type f -name '*.rs')
	$(dir_guard)
	$(RC) $(RUSTFLAGS) $< -o $@

$(BUILD_DIR)/kernel8.elf: $(BUILD_DIR)/start.o $(BUILD_DIR)/main.o $(SRC_DIR)/linker.ld
	$(dir_guard)
	$(LINKER) -T $(SRC_DIR)/linker.ld $(BUILD_DIR)/main.o $(BUILD_DIR)/start.o -o $@

$(BUILD_DIR)/kernel8.img: $(BUILD_DIR)/kernel8.elf
	$(dir_guard)
	$(OBJ_CPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	$(QEMU) $(QEMU_FLAGS) -kernel $(TARGET)

debug: $(TARGET)
	$(QEMU) $(QEMU_FLAGS) -kernel $(TARGET) -S -s
