CC=aarch64-linux-gnu-gcc-10
CFLAGS=-Wall $(INCLUDE_FLAGS) -ffreestanding -g
LD=aarch64-linux-gnu-ld
LD_SCRIPT=linker.ld
OBJCOPY=aarch64-linux-gnu-objcopy

ROOTFS=../../rootfs

.PHONY: clean

all: el0.img

el0.img: el0.elf
	$(OBJCOPY) -O binary el0.elf el0.img
	cp el0.img $(ROOTFS)/el0.img

el0.elf:
	$(CC) $(CFLAGS) -o start.o -c start.S
	$(LD) -T $(LD_SCRIPT) -o el0.elf start.o

clean:
	@rm -rf start.o el0.img el0.elf
