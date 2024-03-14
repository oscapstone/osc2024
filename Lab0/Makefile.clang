CC = clang
LD = ld.lld
OC = llvm-objcopy

CFLAGS := -mcpu=cortex-a53 --target=aarch64-rpi3-elf
LDFLAGS := -m aarch64elf
OCFLAGS := --output-target=aarch64-rpi3-elf -O binary

all: kernel8.img

.PHONY: clean
clean:
	rm -rf *.o *.img

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

ASM_FILES = $(wildcard *.S)
OBJ_FILES = $(ASM_FILES:.S=.o)


kernel8.img: linker.ld $(OBJ_FILES)
	$(LD) $(LDFLAGS) -T $< -o kernel8.elf $(OBJ_FILES)
	$(OC) $(OCFLAGS) kernel8.elf $@
