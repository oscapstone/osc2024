GNU_PREFIX = aarch64-linux-gnu
CC = $(GNU_PREFIX)-gcc
LD = $(GNU_PREFIX)-ld
OC = $(GNU_PREFIX)-objcopy
EXEC = prog1

BUILD_DIR = .
SRC_DIR = .
INC_DIR = .


CFLAGS := -Wall -mcpu=cortex-a53+nosimd -nostdlib -nostdinc -nostartfiles -ffreestanding -mgeneral-regs-only
ASMFLAGS := -I$(INC_DIR) -MMD
OCFLAGS := -O binary

.PHONY: all
all: $(EXEC).img

$(BUILD_DIR)/%_S.o: $(SRC_DIR)/%.S
	$(CC) $(CFLAGS) $(ASMFLAGS) -c $< -o $@

ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_S.o)

$(EXEC).img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(LD) -T $< -o $(BUILD_DIR)/$(EXEC).elf $(OBJ_FILES)
	$(OC) $(OCFLAGS) $(BUILD_DIR)/$(EXEC).elf $@

.PHONY: clean
clean:
	rm -rf $(OBJ_FILES) $(EXEC).img $(EXEC).elf
