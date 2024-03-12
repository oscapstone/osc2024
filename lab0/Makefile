
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

OBJ_DIR = bin-int

OBJ_FILES = $(OBJ_DIR)/src/main.o

all : kernel.img

run : kernel.img
	qemu-system-aarch64 -M raspi3b -kernel $< -S -s

gdb-run :
	gdb-multiarch
	set architecture aarch64
	target remote localhost:1234

kernel.img : kernel.elf
	$(OBJCOPY) -O binary $< $@

kernel.elf : $(OBJ_FILES)
	$(LD) -T linker.ld -o $@ $<

$(OBJ_DIR)/%.o : %.S
	@mkdir -p $(dir $@)
	aarch64-linux-gnu-gcc -c $< -o $@
	
clean :
	rm  kernel.img kernel.elf