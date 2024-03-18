CC 			= clang
LD 			= clang -fuse-ld=lld
OBJCOPY		= llvm-objcopy
QEMU		= qemu-system-aarch64
MINICOM 	= minicom
SERIAL 		= /dev/cu.usbserial-0001

TARGET 		= aarch64-unknown-none-elf
CFLAGS 		= -Wall -Wextra -Wshadow \
			  -ffreestanding \
			  -mcpu=cortex-a53 --target=$(TARGET) \
			  -nostdlib -Os -g \
			  -Iinclude
QEMU_FLAGS 	= -display none \
			  -serial null -serial stdio \
			  -smp cpus=4

BUILD_DIR 	= build
SRC_DIR  	= src

KERNEL_ELF 	= $(BUILD_DIR)/kernel8.elf
KERNEL_BIN 	= kernel8.img

SRCS = $(shell find $(SRC_DIR) -name '*.c')
ASMS = $(shell find $(SRC_DIR) -name '*.S')
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) $(ASMS:$(SRC_DIR)/%.S=$(BUILD_DIR)/%.o)
DEPS = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

.PHONY: all build clean run run-debug upload uart

all: build run

build: $(KERNEL_BIN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(CC) -MMD $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) -MMD $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(SRC_DIR)/linker.ld $(OBJS)
	$(LD) -T $(SRC_DIR)/linker.ld $(CFLAGS) $(OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	$(RM) -r $(KERNEL_BIN) $(BUILD_DIR)

run: $(KERNEL_BIN)
	$(QEMU) -M raspi3b -kernel $(KERNEL_BIN) $(QEMU_FLAGS)

run-debug: QEMU_FLAGS += -s -S
run-debug: run

upload:
	cp kernel8.img /Volumes/BOOT/kernel8.img

uart:
	$(MINICOM) -D $(SERIAL)
