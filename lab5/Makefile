CC = aarch64-linux-gnu
CFLAGS = -ggdb -Wno-implicit -ffreestanding -nostdlib -nostartfiles -Iinclude -mgeneral-regs-only

SRC_DIR = src
INCLUDE_DIR = include
KERNEL_DIR = kernel
LIB_DIR = lib
BOOT_DIR = bootloader
BUILD_DIR = build
IMG_DIR = img

KERNEL_OBJECTS = $(patsubst $(SRC_DIR)/$(KERNEL_DIR)/%.S, $(BUILD_DIR)/$(KERNEL_DIR)/%.o, $(wildcard $(SRC_DIR)/$(KERNEL_DIR)/*.S))
KERNEL_OBJECTS += $(patsubst $(SRC_DIR)/$(KERNEL_DIR)/%.c, $(BUILD_DIR)/$(KERNEL_DIR)/%.o, $(wildcard $(SRC_DIR)/$(KERNEL_DIR)/*.c))

BOOT_OBJECTS = $(patsubst $(SRC_DIR)/$(BOOT_DIR)/%.S, $(BUILD_DIR)/$(BOOT_DIR)/%.o, $(wildcard $(SRC_DIR)/$(BOOT_DIR)/*.S))
BOOT_OBJECTS += $(patsubst $(SRC_DIR)/$(BOOT_DIR)/%.c, $(BUILD_DIR)/$(BOOT_DIR)/%.o, $(wildcard $(SRC_DIR)/$(BOOT_DIR)/*.c))

OBJECTS = $(patsubst $(SRC_DIR)/$(LIB_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/$(LIB_DIR)/*.c))
KERNEL_LIB_OBJECTS = $(patsubst $(SRC_DIR)/$(LIB_DIR)/$(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/$(LIB_DIR)/$(KERNEL_DIR)/*.c))
KERNEL_LIB_OBJECTS += $(patsubst $(SRC_DIR)/$(LIB_DIR)/$(KERNEL_DIR)/%.S, $(BUILD_DIR)/%_s.o, $(wildcard $(SRC_DIR)/$(LIB_DIR)/$(KERNEL_DIR)/*.S))
BOOT_LIB_OBJECTS = $(patsubst $(SRC_DIR)/$(LIB_DIR)/$(BOOT_DIR)/%.c, $(BUILD_DIR)/%.o, $(wildcard $(SRC_DIR)/$(LIB_DIR)/$(BOOT_DIR)/*.c))

HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

ELF_NAME = kernel8.elf
IMG_NAME = kernel8.img
BOOT_ELF_NAME = bootloader.elf
BOOT_IMG_NAME = bootloader.img
CPIO_FILE = initramfs.cpio

all: clean $(OBJECTS) $(KERNEL_LIB_OBJECTS) $(KERNEL_OBJECTS) $(HEADERS)
	$(CC)-ld -T $(SRC_DIR)/$(KERNEL_DIR)/linker.ld -o $(BUILD_DIR)/$(KERNEL_DIR)/$(ELF_NAME) $(KERNEL_OBJECTS) $(KERNEL_LIB_OBJECTS) $(OBJECTS)
	$(CC)-objcopy -O binary $(BUILD_DIR)/$(KERNEL_DIR)/$(ELF_NAME) $(IMG_DIR)/$(IMG_NAME)

boot: $(OBJECTS) $(BOOT_LIB_OBJECTS) $(BOOT_OBJECTS) $(HEADERS)
	$(CC)-ld -T $(SRC_DIR)/$(BOOT_DIR)/linker.ld -o $(BUILD_DIR)/$(BOOT_DIR)/$(BOOT_ELF_NAME) $(BOOT_OBJECTS) $(BOOT_LIB_OBJECTS) $(OBJECTS)
	$(CC)-objcopy -O binary $(BUILD_DIR)/$(BOOT_DIR)/$(BOOT_ELF_NAME) $(IMG_DIR)/$(BOOT_IMG_NAME)

$(BUILD_DIR)/%.o: $(SRC_DIR)/$(LIB_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(IMG_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	@mkdir -p $(BUILD_DIR)/$(BOOT_DIR)
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/$(LIB_DIR)/$(BOOT_DIR)/%.c
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/$(LIB_DIR)/$(KERNEL_DIR)/%.c
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/$(LIB_DIR)/$(KERNEL_DIR)/%.S
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(KERNEL_DIR)/%.o: $(SRC_DIR)/$(KERNEL_DIR)/%.c
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(KERNEL_DIR)/%.o: $(SRC_DIR)/$(KERNEL_DIR)/%.S
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(BOOT_DIR)/%.o: $(SRC_DIR)/$(BOOT_DIR)/%.c
	$(CC)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(BOOT_DIR)/%.o: $(SRC_DIR)/$(BOOT_DIR)/%.S
	$(CC)-gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(IMG_DIR)

dump:
	qemu-system-aarch64 -M raspi3b -kernel $(IMG_DIR)/$(IMG_NAME) -initrd $(CPIO_FILE) -display none -d in_asm -dtb bcm2710-rpi-3-b-plus.dtb

run: all
	qemu-system-aarch64 -M raspi3b -kernel $(IMG_DIR)/$(IMG_NAME) -initrd $(CPIO_FILE) -display none -serial null -serial stdio -s -dtb bcm2710-rpi-3-b-plus.dtb

dump-boot:
	qemu-system-aarch64 -M raspi3b -kernel $(IMG_DIR)/$(BOOT_IMG_NAME) -initrd $(CPIO_FILE) -display none -d in_asm

run-boot:
	qemu-system-aarch64 -M raspi3b -kernel $(IMG_DIR)/$(BOOT_IMG_NAME) -initrd $(CPIO_FILE) -display none -serial null -serial stdio -s

tty:
	qemu-system-aarch64 -M raspi3b -kernel $(IMG_DIR)/$(BOOT_IMG_NAME) -initrd $(CPIO_FILE) -display none -serial null -serial pty

upload:
	sudo python3 upload.py

cpio:
	cd rootfs && find . | cpio -o -H newc > ../$(CPIO_FILE)

screen:
	sudo screen /dev/ttyUSB0 115200

userprog:
	cd user_program && make clean && make
