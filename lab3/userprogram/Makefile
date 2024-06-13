ARMGNU ?= aarch

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -I$(INCLUDE_DIR) -mgeneral-regs-only
ASMOPS = -I$(INCLUDE_DIR)

BUILD_DIR = ../build
INCLUDE_DIR = ../lib/include
LIB_DIR = ../lib

all : user_program.img

clean :
	rm -rf *.d *.o *.elf ../rootfs/user_program.img

%_s.o: %.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_c.o: $(LIB_DIR)/%.c
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(LIB_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

ASM_FILES = $(wildcard *.S)
OBJ_FILES += $(ASM_FILES:%.S=%_s.o)

INC_C_FILES = ../lib/exception.c ../lib/mini_uart.c ../lib/timer.c ../lib/str_util.c
INC_ASM_FILES = ../lib/utils.S
INC_OBJ_FILES = $(INC_C_FILES:../lib/%.c=$(BUILD_DIR)/%_c.o)
INC_OBJ_FILES += $(INC_ASM_FILES:../lib/%.S=$(BUILD_DIR)/%_s.o)

user_program.img: $(OBJ_FILES) $(INC_OBJ_FILES)
	$(ARMGNU)-ld -o user_program.elf $(OBJ_FILES) $(INC_OBJ_FILES)
	$(ARMGNU)-objcopy user_program.elf -O binary ../rootfs/user_program.img