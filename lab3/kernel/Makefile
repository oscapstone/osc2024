GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

# define:
# NS_DEBUG: enable debug output 

# nostdlib: No standard library
CFLAGS	= -Wall -Wextra -Wpedantic -ffreestanding \
		  -nostdlib -nostartfiles \
		  -mgeneral-regs-only\
		  -fno-stack-protector\
		  -O0\
		  -I src\
		  -D NS_DEBUG

BIN_DIR = bin

OBJ_DIR = bin-int

# object files
OBJ_FILES = $(addprefix $(OBJ_DIR)/, src/start.o src/main.o \
				src/io/uart.o src/io/dtb.o src/io/reboot.o src/io/exception.o\
				src/utils/utils.o src/utils/utilsASM.o src/utils/printf.o src/utils/fifo_buffer.o\
				src/shell/shell.o \
				src/fs/cpio.o \
				src/peripherals/gpio.o src/peripherals/irq.o src/peripherals/timer.o\
				)

# the directories of the output files
OUT_DIRS = $(sort $(dir $(OBJ_FILES)))

QEMU_DEVICE = /dev/pts/2

all : kernel.img

gdb-run :
	gdb-multiarch -ex 'set architecture aarch64' -ex 'file bin/kernel.elf' -ex 'target remote localhost:1234'

upload : all
	python3 uploader_qemu.py $(QEMU_DEVICE)
	sudo screen $(QEMU_DEVICE) 115200

kernel.img : $(OBJ_FILES) linker.ld $(BIN_DIR)
	$(LD) --verbose $(OBJ_FILES) -T linker.ld -o $(BIN_DIR)/kernel.elf
	$(OBJCOPY) -O binary $(BIN_DIR)/kernel.elf $@

clean :
	rm -rf $(OBJ_FILES) kernel.img $(BIN_DIR)/kernel.elf

$(OBJ_DIR)/%.o : %.S | $(OUT_DIRS)
	$(GCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : %.c $(OUT_DIRS)
	$(GCC) $(CFLAGS) -c $< -o $@

# directories to be make
$(OUT_DIRS):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@





