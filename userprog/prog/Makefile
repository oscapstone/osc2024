# This Makefile is used to compile the user program

PROG_IMAGE = target/prog
PROG_ELF = target/prog.elf
PROG_SRC = src/prog.s
LD = src/prog.ld
SRC = $(PROG_SRC) $(LD)
CC = aarch64-linux-gnu-gcc

.PHONY: all

all: $(PROG_IMAGE)

$(PROG_IMAGE): $(PROG_ELF)
	@echo "Generating stripped binary"
	rust-objcopy --strip-all -O binary $(PROG_ELF) $(PROG_IMAGE)
	@echo "Name"
	@echo $(PROG_IMAGE)
	@echo "Size"
	@du -h $(PROG_IMAGE)

$(PROG_ELF): $(SRC)
	$(CC) -c $(PROG_SRC) -o $(PROG_ELF) -T $(LD)

clean:
	rm -f target/*
