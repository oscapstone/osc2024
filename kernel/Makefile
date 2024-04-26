### variables
CC = clang
LD = ld.lld
OBJCOPY = llvm-objcopy

SRC_DIR = src
OBJ_DIR = obj
SUB_DIR = command

SRC_FILES= $(shell find $(SRC_DIR) -type f -name "*.c")
# for files in SRC_FILES, turn them from src/*.c to obj/*.o
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJ_FILES += $(OBJ_DIR)/start.o

DEP_FILES = $(OBJ_FILES:%.o=%.d)

# flags
INCLUDE = -Iinclude -I../lib/include
TARGET = -mcpu=cortex-a53 --target=aarch64-rpi3-elf 
CFLAGS = -Wall -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc -MMD $(TARGET) $(INCLUDE)

# lib
LIBPATH = ../lib
LIBFLAG = -L../lib -lab
LIB = ../lib/libab.a

# ramdisk
INITRD = initramfs.cpio
DTB = bcm2710-rpi-3-b-plus.dtb

# qemu
QEMU_FLAG = -M raspi3b -display none \
			-initrd $(INITRD) -dtb $(DTB) \
			-serial null -serial stdio

ifeq ($(DEBUG), 1)
	CFLAGS += -g
	QEMU_FLAG += -s -S
endif


ifeq ($(QEMU), 1)
	CFLAGS += -D__QEMU__
endif

### dependencies
.PHONY: all
all: kernel8.img $(OBJ_DIR)

-include $(DEP_FILES)

$(OBJ_DIR)/start.o: start.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.img: $(OBJ_FILES) $(LIB)
	$(LD) -m aarch64elf -nostdlib $(OBJ_FILES) -T linker.ld -o kernel8.elf $(LIBFLAG)
	$(OBJCOPY) -O binary kernel8.elf $@

.PHONY: $(LIB)
$(LIB):
	$(MAKE) -C $(LIBPATH)


initramfs.cpio: rootfs
	cd $< && find . | cpio -o -H newc > ../$@


### actions
.PHONY: clean
clean:
	-@rm -v kernel8.*
	-@rm -rv obj
	-@rm -v initramfs.cpio

.PHONY: run
run: kernel8.img initramfs.cpio
	qemu-system-aarch64 $(QEMU_FLAG) -kernel kernel8.img 

.PHONY: debug
debug: kernel8.img initramfs.cpio
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -S -s -display none
