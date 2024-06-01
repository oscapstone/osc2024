GCC		= aarch64-linux-gnu-gcc
LD		= aarch64-linux-gnu-ld
OBJCOPY	= aarch64-linux-gnu-objcopy

# define:
# NS_DEBUG: enable debug output 

# nostdlib: No standard library
CFLAGS	= -Wall -Wextra -Wpedantic -ffreestanding \
		  -nostdlib -nostartfiles \
		  -mgeneral-regs-only\
		  -fno-stack-protector\
		  -O0\
		  -I src -I lib\
		  -D NS_DEBUG

LIB		= lib/fork.o lib/uartwrite.o lib/printf.o lib/getpid.o start.o lib/exit.o lib/mbox_call.o

all : testProgram test_fork test_cow test_mbox

clean :
	rm -rf testProgram test_fork test_cow

cpio :
	find . | cpio -o -H newc > ../initramfs.cpio

test_fork : test_fork.o $(LIB)
	$(LD) $^ -T linker.ld -o test_fork.elf
	$(OBJCOPY) -O binary test_fork.elf $@

test_cow : test_cow.o $(LIB)
	$(LD) $^ -T linker.ld -o test_cow.elf
	$(OBJCOPY) -O binary test_cow.elf $@

test_mbox : test_mbox.o $(LIB)
	$(LD) $^ -T linker.ld -o test_mbox.elf
	$(OBJCOPY) -O binary test_mbox.elf $@

testProgram : testProgram.o lib/fork.o
	$(LD) testProgram.o -T linker.ld -o testProgram.elf
	$(OBJCOPY) -O binary testProgram.elf $@

%.o : %.S
	$(GCC) $(CFLAGS) -c $< -o $@

%.o : %.c
	$(GCC) $(CFLAGS) -c $< -o $@