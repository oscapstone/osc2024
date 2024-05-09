# Specify the cross-compiler prefix
ARMGNU ?= aarch64-linux-gnu

# Compiler options, donn't use C standard library, don't use standard startup files,
# -ffreestanding directs the compiler to not assume that standard functions have their usual definition
COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -g
# Assembler options
ASMOPS = -Iinclude 

# Directories for build output and source files
BUILD_DIR = build
SRC_DIR = src

# Default target: build the kernel image
all : kernel8.img

# Clean target: remove build artifacts and the kernel image
clean :
	rm -rf $(BUILD_DIR) *.img 

# Rule to compile C source files
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

# Rule to compile assembly source files
$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

# Lists of C and assembly source files
# wildcard get the name of all specific files and return as a list
C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
# Lists of corresponding object files
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

# Dependency files for automatic header dependency tracking
DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

# Rule to link the object files into an ELF executable and then convert it to a binary image
# The -MMD option in the GCC compiler is used to generate dependency files during compilation. 
# Specifically, it creates a .d file for each .c file that is compiled, which lists all the header files that the source file depends on.
kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

# Rule to run the kernel in QEMU
run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio

# Rule to start QEMU in debug mode for use with GDB
gdb:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -s -S 