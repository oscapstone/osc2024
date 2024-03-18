CXX 		= clang++
LD 			= clang -fuse-ld=lld
OBJCOPY		= llvm-objcopy
QEMU		= qemu-system-aarch64
MINICOM 	= minicom
SERIAL 		= /dev/cu.usbserial-0001

CFLAGS 		= -Wall -Wextra -Wshadow \
			  -ffreestanding \
			  -mcpu=cortex-a53 \
			  --target=aarch64-unknown-none-elf \
			  -D_LIBCPP_HAS_NO_THREADS \
			  -nostdlib -Os -fPIE
QEMU_FLAGS 	= -display none \
			  -serial null -serial stdio \
			  -smp cpus=4

BUILD_DIR 	= build
SRC_DIR  	= src

ifeq ($(TARGET),)
	TARGET = kernel
endif

ifneq ($(DEBUG),)
	CFLAGS 		+= -g
	QEMU_FLAGS 	+= -s -S
endif

TARGET_BUILD_DIR 	= $(BUILD_DIR)/$(TARGET)
TARGET_SRC_DIR  	= $(SRC_DIR)/$(TARGET)
CFLAGS 				+= -Iinclude/$(TARGET)

KERNEL_ELF 	= $(TARGET_BUILD_DIR)/$(TARGET).elf
KERNEL_BIN 	= $(BUILD_DIR)$(TARGET).img

SRCS = $(shell find $(TARGET_SRC_DIR) -name '*.cpp')
ASMS = $(shell find $(TARGET_SRC_DIR) -name '*.S')
OBJS = $(SRCS:$(TARGET_SRC_DIR)/%.c=$(TARGET_BUILD_DIR)/%.o) $(ASMS:$(TARGET_SRC_DIR)/%.S=$(TARGET_BUILD_DIR)/%.o)
DEPS = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

.PHONY: all build clean run run-debug upload uart

all: kernel

kernel:
	$(MAKE) TARGET=kernel build run

build: $(KERNEL_BIN)

$(TARGET_BUILD_DIR)/%.o: $(TARGET_SRC_DIR)/%.S
	@mkdir -p $(@D)
	$(CXX) -MMD $(CFLAGS) -c $< -o $@

$(TARGET_BUILD_DIR)/%.o: $(TARGET_SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) -MMD $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(TARGET_SRC_DIR)/linker.ld $(OBJS)
	$(LD) -T $(TARGET_SRC_DIR)/linker.ld $(CFLAGS) $(OBJS) -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	$(RM) -r $(BUILD_DIR)

run: $(KERNEL_BIN)
	$(QEMU) -M raspi3b -kernel $(KERNEL_BIN) $(QEMU_FLAGS)

upload:
	cp $(KERNEL_BIN) /Volumes/BOOT/kernel8.img

uart:
	$(MINICOM) -D $(SERIAL)
