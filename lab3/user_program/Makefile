all: user1.img

user1.o: user1.S
	aarch64-linux-gnu-gcc -c user1.S -o user1.o

user1.img: user1.o
	aarch64-linux-gnu-ld user1.o -T linker.ld -o user1.elf
	aarch64-linux-gnu-objcopy -O binary user1.elf user1.img
	mv user1.img ../rootfs  

clean:
	rm -rf user1.o user1.elf user1.img