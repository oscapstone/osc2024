ARMGNU ?= aarch

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -I../lib/include -mgeneral-regs-only
ASMOPS = -I../lib/include

BUILD_DIR = ../build

all : bootloader.img

clean :
	rm -rf $(BUILD_DIR)
	rm -rf bootloader.img

$(BUILD_DIR)/%_c.o: %.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: %.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

C_FILES = $(wildcard *.c)
ASM_FILES = $(wildcard *.S)
OBJ_FILES = $(C_FILES:%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:%.S=$(BUILD_DIR)/%_s.o)

INC_C_FILES = \
		../lib/mini_uart.c \
		../lib/reboot.c \
		../lib/str_util.c \
		../lib/exception.c \
		../lib/timer.c \
		
INC_ASM_FILES = ../lib/mm.S ../lib/utils.S
INC_OBJ_FILES = $(INC_C_FILES:../lib/%.c=$(BUILD_DIR)/%_c.o)
INC_OBJ_FILES += $(INC_ASM_FILES:../lib/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

bootloader.img: linker.ld $(OBJ_FILES) $(INC_OBJ_FILES)
	$(ARMGNU)-ld -T linker.ld -o $(BUILD_DIR)/bootloader.elf $(OBJ_FILES) $(INC_OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/bootloader.elf -O binary bootloader.img

