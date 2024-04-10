.PHONY: all clean

CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

TARGET = prog.img

all: prog.img


prog.elf: prog.o prog.ld
	$(LD) -T prog.ld -o $@ prog.o

prog.o: prog.S
	$(CC) -c -o $@ $<

prog.img: prog.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f prog.o prog.elf
