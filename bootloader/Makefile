CC = clang
LD = ld.lld
OBJCOPY = llvm-objcopy

SRCDIR = src
OBJDIR = obj

SRCFILES = $(wildcard $(SRCDIR)/*.c)
OBJFILES = $(SRCFILES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# flags
INCLUDE = -Iinclude -I../lib/include
TARGET = -mcpu=cortex-a53 --target=aarch64-rpi3-elf 
CFLAGS = -Wall -nostdlib -ffreestanding -mgeneral-regs-only -nostdinc $(TARGET) $(INCLUDE)
ifeq ($(DEBUG), 1)
	CFLAGS += -g
endif


LINKFLAGS = -m aarch64elf -nostdlib

# lib
LIBPATH = ../lib
LIBFLAG = -L../lib -lab
LIB = ../lib/libab.a

LINKFLAGS += $(LIBFLAG)

.PHONY: all
all: bootloader.img

bootloader.img: bootloader.elf
	$(OBJCOPY) -O binary $< $@

bootloader.elf: $(OBJDIR)/start.o $(OBJFILES) $(LIB)
	$(LD) $(LINKFLAGS) $(filter-out $(LIB), $^) -T linker.ld -o $@

$(OBJDIR)/start.o: start.S $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB):
	$(MAKE) -C $(LIBPATH)

$(OBJDIR):
	mkdir $@


.PHONY: run
run: bootloader.img
	qemu-system-aarch64 -M raspi3b -kernel $< -serial null -serial pty -serial stdio


.PHONY: debug
debug: bootloader.img
	qemu-system-aarch64 -M raspi3b -kernel $< -S -s -display none

.PHONY: clean
clean:
	-@rm -rv $(OBJDIR)
	-@rm -v bootloader.img bootloader.elf


