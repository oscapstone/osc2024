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

LIB		= lib/fork.o lib/uartwrite.o lib/printf.o lib/getpid.o start.o

all : testProgram test_fork test_tfault

clean :
	rm -rf testProgram test_fork test_tfault

test_fork : test_fork.o lib/fork.o lib/uartwrite.o start.o
	$(LD) $^ -T linker.ld -o test_fork.elf
	$(OBJCOPY) -O binary test_fork.elf $@

test_tfault : test_tfault.o $(LIB)
	$(LD) $^ -T linker.ld -o test_tfault.elf
	$(OBJCOPY) -O binary test_tfault.elf $@

testProgram : testProgram.o lib/fork.o
	$(LD) testProgram.o -T linker.ld -o testProgram.elf
	$(OBJCOPY) -O binary testProgram.elf $@

%.o : %.S
	$(GCC) $(CFLAGS) -c $< -o $@

%.o : %.c
	$(GCC) $(CFLAGS) -c $< -o $@