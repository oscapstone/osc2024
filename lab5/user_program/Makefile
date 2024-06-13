CC = aarch64-linux-gnu
CFLAGS = -ggdb -Wno-implicit -ffreestanding -nostdlib -nostartfiles -Iinclude

BUILD_DIR = build
ROOTFS_DIR = ../rootfs

OBJECTS = $(patsubst %.S, $(BUILD_DIR)/%.o, $(wildcard *.S))

USER_PROG = userprog

all: $(OBJECTS)
	$(CC)-ld -T linker.ld -o $(BUILD_DIR)/$(USER_PROG).elf $(OBJECTS)
	$(CC)-objcopy -O binary $(BUILD_DIR)/$(USER_PROG).elf $(ROOTFS_DIR)/$(USER_PROG)

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(BUILD_DIR)
	$(CC)-gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ROOTFS_DIR)/$(USER_PROG)

run:
	qemu-system-aarch64 -M raspi3b -kernel $(ROOTFS_DIR)/$(USER_PROG) -display none -serial null -serial stdio -s
