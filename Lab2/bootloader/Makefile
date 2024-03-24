ver = release
GNU_PREFIX = aarch64-linux-gnu
CC = $(GNU_PREFIX)-gcc
LD = $(GNU_PREFIX)-ld
OC = $(GNU_PREFIX)-objcopy
EMU = qemu-system-aarch64
DB = gdb-multiarch

BUILD_DIR = build
SRC_DIR = src
INC_DIR = include

CFLAGS := -Wall -mcpu=cortex-a53+nosimd -nostdlib -nostdinc -nostartfiles -ffreestanding -mgeneral-regs-only
CPPFLAGS := -I$(INC_DIR) -MMD
ASMFLAGS := -I$(INC_DIR) -MMD
OCFLAGS := -O binary
EMUFLAGS := -M raspi3b -serial null -serial stdio -display none

ifeq ($(ver), debug)
CFLAGS += -g
CPPFLAGS += -DDEBUG
ASMFLAGS += -DDEBUG
EMUFLAGS += -S -s

debug:  bootloader.img
	$(EMU) $(EMUFLAGS) -kernel $< &
	$(DB) $(BUILD_DIR)/bootloader.elf
endif

.DELETE_ON_ERROR: all
all: bootloader.img

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

	
bootloader.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(LD) -T $< -o $(BUILD_DIR)/bootloader.elf $(OBJ_FILES)
	$(OC) $(OCFLAGS) $(BUILD_DIR)/bootloader.elf $@

.PHONY: emu
emu: bootloader.img
	$(EMU) $(EMUFLAGS) -kernel $<
