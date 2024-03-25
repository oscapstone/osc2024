SRC_DIR = src
BUILD_DIR = build
RPI3_DIR = rpi3

CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall -static

RC = rustc
RUSTFLAGS = --crate-type=staticlib --emit=obj --target=aarch64-unknown-linux-gnu -C panic=abort -C opt-level=3

LINKER = aarch64-linux-gnu-ld
LINKER_FLAGS = -static
OBJ_CPY = aarch64-linux-gnu-objcopy

QEMU = qemu-system-aarch64
QEMU_FLAGS = -M raspi3b -serial null -serial pty

TARGET = $(BUILD_DIR)/kernel8.img $(BUILD_DIR)/bootloader.img

dir_guard=@mkdir -p $(@D)

.PHONY: all clean run debug

all: $(TARGET) $(BUILD_DIR)/initramfs.cpio
	cp $^ $(RPI3_DIR)
	sha1sum $(TARGET)

$(BUILD_DIR)/start.o: $(SRC_DIR)/start.s
	$(dir_guard)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bootloader.o: $(SRC_DIR)/bootloader.rs $(shell find $(SRC_DIR)/ -type f -name '*.rs')
	$(dir_guard)
	$(RC) $(RUSTFLAGS) $< -o $@


$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel.rs $(shell find $(SRC_DIR)/ -type f -name '*.rs')
	$(dir_guard)
	$(RC) $(RUSTFLAGS) $< -o $@

$(BUILD_DIR)/bootloader.elf: $(BUILD_DIR)/start.o $(BUILD_DIR)/bootloader.o $(SRC_DIR)/bootloader.ld
	$(dir_guard)
	$(LINKER) $(LINKER_FLAGS) -T $(SRC_DIR)/bootloader.ld $(BUILD_DIR)/bootloader.o $(BUILD_DIR)/start.o -o $@

$(BUILD_DIR)/kernel8.elf:$(BUILD_DIR)/kernel.o $(SRC_DIR)/kernel.ld
	$(dir_guard)
	$(LINKER) $(LINKER_FLAGS) -T $(SRC_DIR)/kernel.ld $(BUILD_DIR)/kernel.o -o $@

$(BUILD_DIR)/bootloader.img: $(BUILD_DIR)/bootloader.elf
	$(dir_guard)
	$(OBJ_CPY) -O binary $< $@

$(BUILD_DIR)/kernel8.img: $(BUILD_DIR)/kernel8.elf
	$(dir_guard)
	$(OBJ_CPY) -O binary $< $@

$(BUILD_DIR)/initramfs.cpio: rootfs
	cd rootfs && find . | cpio -o -H newc > ../$@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	$(QEMU) $(QEMU_FLAGS) -kernel $(BUILD_DIR)/bootloader.img

debug: $(TARGET)
	$(QEMU) $(QEMU_FLAGS) -kernel $(BUILD_DIR)/bootloader.img -S -s
