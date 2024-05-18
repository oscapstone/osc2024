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
		  -I src\
		  -D NS_DEBUG

all : testProgram

testProgram : testProgram.o
	$(LD) testProgram.o -T linker.ld -o testProgram.elf
	$(OBJCOPY) -O binary testProgram.elf $@

%.o : %.S
	$(GCC) $(CFLAGS) -c $< -o $@