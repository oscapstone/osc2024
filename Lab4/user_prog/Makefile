CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -g

ASM_FILES = $(wildcard *.S)
OBJS = $(ASM_FILES:%.S=%.o)

all: clean user_prog

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

user_prog: $(OBJS)
	$(LD) -static -o user_prog.elf $(OBJS)
	$(OBJCOPY) user_prog.elf -O binary user_prog.img
	cp ./user_prog.img ../rootfs/

clean:
	rm *.o *.elf *.img