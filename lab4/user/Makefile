all: user.img

user.o: user.S
	aarch64-linux-gnu-gcc -c user.S -o user.o

user.img: user.o
	aarch64-linux-gnu-ld user.o -T link.ld -o user.elf
	aarch64-linux-gnu-objcopy -O binary user.elf user.img

clean:
	rm -f *.o *.elf *.img
