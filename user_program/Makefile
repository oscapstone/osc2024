TOOLCHAIN = aarch64-none-elf
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -ffreestanding -nostdinc -nostdlib -nostartfiles

all: user

user:
	${TOOLCHAIN}-gcc $(CFLAGS) -c user.S -o user.o
	${TOOLCHAIN}-ld -nostdlib  user.o -T linker.ld -o user.elf
	${TOOLCHAIN}-objcopy -O binary user.elf user.img
clean:
	rm -f *.o *.elf *.img