TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld


LINKER_FILE = linker.ld
CFLAGS = -c -Wall -O2 -nostdinc -nostdlib -nostartfiles -ffreestanding -fno-stack-protector

.PHONY: all clean 

all: user_prog.out
	
user_prog.out: user_prog.o
	$(LD) -T $(LINKER_FILE) -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *.out
