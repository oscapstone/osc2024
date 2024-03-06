CC = clang
LD = ld.lld
OC = llvm-objcopy

BUILD_DIR = build
SRC_DIR = src
INC_DIR = include

TARGET = aarch64-rpi3-elf
CFLAGS := -Weverything -mcpu=cortex-a53+nosimd --target=$(TARGET) -nostdlib -nostdinc -nostartfiles -ffreestanding -mgeneral-regs-only
CPPFLAGS := -I$(INC_DIR) -MMD
ASMFLAGS := -I$(INC_DIR) -MMD
LDFLAGS := -m aarch64elf -nostdlib
OCFLAGS := --output-target=$(TARGET) -O binary

.DELETE_ON_ERROR:
all: kernel8.img

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *.img

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%_S.o: $(SRC_DIR)/%.S
	$(CC) $(CFLAGS) $(ASMFLAGS) -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_S.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)

-include $(DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(LD) $(LDFLAGS) -T $< -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(OC) $(OCFLAGS) $(BUILD_DIR)/kernel8.elf $@
