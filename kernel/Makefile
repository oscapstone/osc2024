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

# flags
INCLUDE = -Iinclude -I../lib/include
TARGET = -mcpu=cortex-a53 --target=aarch64-rpi3-elf 
CFLAGS = -g -Wall -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc $(TARGET) $(INCLUDE)

# lib
LIBPATH = ../lib
LIBFLAG = -L../lib -lab
LIB = ../lib/libab.a

ifeq ($(QEMU), 1)
	CFLAGS += -D__QEMU__
endif

### dependencies
.PHONY: all
all: kernel8.img

start.o: start.S
	$(CC) $(CFLAGS) -c start.S -o start.o

$(OBJ_DIR):
	mkdir $(OBJ_DIR)
	mkdir $(OBJ_DIR)/$(SUB_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJ_FILES) $(LIB)
	$(LD) -m aarch64elf -nostdlib start.o $(OBJ_FILES) -T linker.ld -o kernel8.elf $(LIBFLAG)
	$(OBJCOPY) -O binary kernel8.elf kernel8.img

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
	-@rm -v start.o
	-@rm -v initramfs.cpio

.PHONY: run
run: kernel8.img initramfs.cpio
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -serial null -serial stdio

.PHONY: debug
debug: kernel8.img initramfs.cpio
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -S -s -display none
