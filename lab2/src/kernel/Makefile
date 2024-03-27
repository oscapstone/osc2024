
GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

CFLAGS	= -Wall -Wextra -Wpedantic -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

SRC_FILES = $(wildcard *.c)

OBJ_DIR = bin-int

OBJ_FILES = $(SRC_FILES:.c=.o) start.o

all : kernel.img

kernel.img : $(OBJ_FILES)
	$(LD) $^ -T linker.ld -o kernel.elf
	$(OBJCOPY) -O binary kernel.elf $@

uploadQEMU:
	python uploader_qemu.py

upload:
	python uploader.py

clean:
	rm $(OBJ_FILES) kernel.img

%.o : %.S
	$(GCC) $(CFLAGS) -c $< -o $@

%.o : %.c
	$(GCC) $(CFLAGS) -c $< -o $@

