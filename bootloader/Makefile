CC = clang
LD = ld.lld
OBJCOPY = llvm-objcopy

SRC_DIR = src
OBJ_DIR = obj

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEP_FILES = $(OBJ_FILES:%.o=%.d)


KERNEL_IMG = ../kernel/kernel8.img
INITRD = ../kernel/initramfs.cpio
DTB = ../kernel/bcm2710-rpi-3-b-plus.dtb

# flags
INCLUDE = -Iinclude -I../lib/include
TARGET = -mcpu=cortex-a53 --target=aarch64-rpi3-elf 
CFLAGS = -Wall -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc -MMD $(TARGET) $(INCLUDE)

LINKFLAGS = -m aarch64elf -nostdlib

# lib
LIBPATH = ../lib
LIBFLAG = -L../lib -lab
LIB = ../lib/libab.a

LINKFLAGS += $(LIBFLAG)

# qemu
QEMU_FLAG = -M raspi3b -display none \
			-initrd $(INITRD) -dtb $(DTB) \
			-serial null -serial pty

ifeq ($(DEBUG), 1)
	CFLAGS += -g
	QEMU_FLAG += -s -S
endif


.PHONY: all
all: bootloader.img

bootloader.img: bootloader.elf
	$(OBJCOPY) -O binary $< $@

bootloader.elf: $(OBJ_DIR)/start.o $(OBJ_FILES) $(LIB)
	$(LD) $(LINKFLAGS) $(filter-out $(LIB), $^) -T linker.ld -o $@

-include $(DEP_FILES)

$(OBJ_DIR)/start.o: start.S 
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB):
	$(MAKE) -C $(LIBPATH)

.PHONY: $(KERNEL_IMG)
$(KERNEL_IMG):
	make -C ../kernel


.PHONY: run
run: bootloader.img $(KERNEL_IMG)
	qemu-system-aarch64 $(QEMU_FLAG) -kernel $< 


.PHONY: clean
clean:
	-@rm -rv $(OBJ_DIR)
	-@rm -v bootloader.img bootloader.elf


