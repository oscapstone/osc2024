CXX 		= clang++
LD 			= clang -fuse-ld=lld
OBJCOPY		= llvm-objcopy
QEMU		= qemu-system-aarch64
MINICOM 	= minicom
SERIAL 		= /dev/cu.usbserial-0001

BUILD_DIR 	= build
SRC_DIR  	= src
DISK_DIR 	= disk
FS_DIR 		= rootfs

CFLAGS 		= -Wall -Wextra -Wshadow \
			  -ffreestanding \
			  -mcpu=cortex-a53 -mgeneral-regs-only \
			  --target=aarch64-unknown-none-elf \
			  -D_LIBCPP_HAS_NO_THREADS \
			  -D_LIBCPP_DISABLE_AVAILABILITY \
			  -D_LIBCPP_CSTDLIB \
			  -fno-exceptions \
			  -std=c++20 \
			  -nostdlib -Os -fPIE
QEMU_FLAGS 	= -display none -smp cpus=4 \
			  -dtb $(DISK_DIR)/bcm2710-rpi-3-b-plus.dtb \
			  $(QEMU_EXT_FLAGS)

ifeq ($(TARGET),)
	TARGET = kernel
endif

ifneq ($(DEBUG),)
	CFLAGS 		+= -g
	QEMU_FLAGS 	+= -s -S
endif

ifeq ($(QEMU_PTY_SERIAL),)
	QEMU_FLAGS 	+= -serial null -serial stdio
else
	QEMU_FLAGS 	+= -serial null -serial pty
endif

LIB_SRC_DIR  	= $(SRC_DIR)/lib
CFLAGS 			+= -Iinclude/lib

TARGET_BUILD_DIR 	= $(BUILD_DIR)/$(TARGET)
TARGET_SRC_DIR  	= $(SRC_DIR)/$(TARGET)
CFLAGS 				+= -Iinclude/$(TARGET)

KERNEL_ELF 	= $(BUILD_DIR)/$(TARGET).elf
KERNEL_BIN 	= $(DISK_DIR)/$(TARGET).img
LINKER 		= $(TARGET_SRC_DIR)/linker.ld
INITFSCPIO 	= $(DISK_DIR)/initramfs.cpio
QEMU_FLAGS 	+= -initrd $(INITFSCPIO)

SRCS = $(shell find $(TARGET_SRC_DIR) $(LIB_SRC_DIR) -name '*.cpp')
ASMS = $(shell find $(TARGET_SRC_DIR) $(LIB_SRC_DIR) -name '*.S')
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o) $(ASMS:$(SRC_DIR)/%.S=$(BUILD_DIR)/%-asm.o)
DEPS = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

.PHONY: all build fs clean run run-debug upload disk disk-format uart

all: build run

kernel:
	$(MAKE) build TARGET=kernel

bootloader:
	$(MAKE) build TARGET=bootloader

build: $(KERNEL_BIN)

$(BUILD_DIR)/%-asm.o: $(SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(CXX) -MMD $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -MMD $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(LINKER) $(OBJS)
	$(LD) -T $(LINKER) $(CFLAGS) $(OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

fs: $(INITFSCPIO)

$(INITFSCPIO): $(shell find $(FS_DIR))
	$(MAKE) -C $(FS_DIR)
	cd $(FS_DIR) && find . | grep -v '.DS_Store' | cpio -o -H newc > ../$@

clean:
	$(RM) -r $(BUILD_DIR)

run: $(KERNEL_BIN)
	$(MAKE) -C $(DISK_DIR) dtb
	$(QEMU) -M raspi3b -kernel $(KERNEL_BIN) $(QEMU_FLAGS)

upload:
	./script/upload.py $(SERIAL)

disk:
	$(MAKE) -C $(DISK_DIR) upload eject

uart:
	$(MINICOM) -D $(SERIAL)
