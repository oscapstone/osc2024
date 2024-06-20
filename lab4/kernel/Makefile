ARMGNU ?= aarch

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -I$(INCLUDE_DIR) -mgeneral-regs-only
ASMOPS = -I$(INCLUDE_DIR)

INCLUDE_DIR = ../lib/include
BUILD_DIR = ../build
LIB_DIR = ../lib

all : kernel8.img

clean :
	rm -rf $(BUILD_DIR)
	rm -rf kernel8.elf kernel8.img

$(BUILD_DIR)/%_c.o: %.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@ -g

$(BUILD_DIR)/%_s.o: %.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@ -g

$(BUILD_DIR)/%_c.o: $(LIB_DIR)/%.c
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@ -g

$(BUILD_DIR)/%_s.o: $(LIB_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@ -g

C_FILES = $(wildcard *.c)
ASM_FILES = $(wildcard *.S)
OBJ_FILES = $(C_FILES:%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:%.S=$(BUILD_DIR)/%_s.o)

INC_C_FILES = \
		../lib/cpio.c \
		../lib/device_tree.c \
		../lib/mailbox.c \
		../lib/malloc.c \
		../lib/mini_uart.c \
		../lib/reboot.c \
		../lib/str_util.c \
		../lib/exception.c \
		../lib/timer.c \
		../lib/buddy.c \
		../lib/dyn_alloc.c \
		
INC_ASM_FILES = ../lib/mm.S ../lib/utils.S
INC_OBJ_FILES = $(INC_C_FILES:../lib/%.c=$(BUILD_DIR)/%_c.o)
INC_OBJ_FILES += $(INC_ASM_FILES:../lib/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: linker.ld $(OBJ_FILES) $(INC_OBJ_FILES)
	$(ARMGNU)-ld -T linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES) $(INC_OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img