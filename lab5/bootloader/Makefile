GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

CFLAGS	= -Wall -Wextra -Wpedantic -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

BIN_DIR = bin

OBJ_DIR = bin-int

# object files
OBJ_FILES = $(addprefix $(OBJ_DIR)/, start.o io/uart.o main.o)

# the directories of the output files
OUT_DIRS = $(sort $(dir $(OBJ_FILES)))

all : bootloader.img

bootloader.img : $(OBJ_FILES) linker.ld $(BIN_DIR)
	$(LD) $(OBJ_FILES) -T linker.ld -o $(BIN_DIR)/bootloader.elf
	$(OBJCOPY) -O binary $(BIN_DIR)/bootloader.elf $@

run : bootloader.img
	# qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none -device loader,file=$<,addr=0x80000,cpu-num=0
	qemu-system-aarch64 -M raspi3b -serial null -serial pty -kernel $< -initrd ../initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb
	
debugRun : bootloader.img
	# qemu-system-aarch64 -M raspi3b -serial null -serial pty -display none -device loader,file=$<,addr=0x80000,cpu-num=0 -S -s
	qemu-system-aarch64 -M raspi3b -serial null -serial pty -kernel $< -initrd ../initramfs.cpio -S -s -dtb bcm2710-rpi-3-b-plus.dtb -drive if=sd,file=sfn_nctuos.img,format=raw

gdb-run :
	gdb-multiarch -ex 'set architecture aarch64' -ex 'file bin/bootloader.elf' -ex 'target remote localhost:1234'

clean :
	rm $(OBJ_FILES) bootloader.img $(BIN_DIR)/bootloader.elf

$(OBJ_DIR)/%.o : %.S | $(OUT_DIRS)
	$(GCC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o : %.c $(OUT_DIRS)
	$(GCC) $(CFLAGS) -c $< -o $@

# directories to be make
$(OUT_DIRS):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@





