
GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

OBJ_DIR = bin-int

OBJ_FILES = $(OBJ_DIR)/src/start.o $(OBJ_DIR)/src/main.o $(OBJ_DIR)/src/uart.o $(OBJ_DIR)/src/utils.o $(OBJ_DIR)/src/shell.o

CFLAGS	= -Wall -Wextra -Wpedantic -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

all : kernel.img

run : kernel.img
	qemu-system-aarch64 -M raspi3b -serial stdio -kernel $< -S -s -display none -d in_asm

gdb-run :
	gdb-multiarch
	set architecture aarch64
	file kernel.elf
	target remote localhost:1234

kernel.img : kernel.elf
	$(OBJCOPY) -O binary $< $@

kernel.elf : $(OBJ_FILES)
	$(LD) --verbose -T linker.ld -o $@ $^

$(OBJ_DIR)/%.o : %.S
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : %.c
	@mkdir -p $(dir $@)
	$(GCC) $(CFLAGS) -c $< -o $@
	

clean :
	rm  kernel.img kernel.elf