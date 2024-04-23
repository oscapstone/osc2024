GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

# define:
# NS_DEBUG: enable debug output 

# nostdlib: No standard library
CFLAGS	= -Wall -Wextra -Wpedantic -O0 -ffreestanding \
		  -nostdinc -nostdlib -nostartfiles \
		  -mgeneral-regs-only\
		  -I src\
		  -D NS_DEBUG

BIN_DIR = bin

OBJ_DIR = bin-int

# object files
OBJ_FILES = $(addprefix $(OBJ_DIR)/, src/start.o src/main.o \
				src/io/uart.o src/io/dtb.o src/io/reboot.o\
				src/utils/utils.o src/utils/utilsASM.o \
				src/shell/shell.o \
				src/fs/cpio.o \
				src/peripherals/gpio.o\
				)

# the directories of the output files
OUT_DIRS = $(sort $(dir $(OBJ_FILES)))

all : kernel.img

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





