ARMGNU ?= aarch64-linux-gnu

all: user_prog.img

user_prog.img: user_prog.o
	$(ARMGNU)-ld -o user_prog.elf user_prog.o
	$(ARMGNU)-objcopy user_prog.elf -O binary user_prog.img
	cp ./user_prog.img ../rootfs/
	(cd ../rootfs/ && find . | cpio -o -H newc > ../initramfs.cpio)

user_prog.o: user_prog.S
	$(ARMGNU)-gcc -MMD -c $< -o $@

clean:
	rm -f *.o *.d *.elf *.img

dump: user_prog.img
	qemu-aarch64 -M raspi3 -kernel user_prog.img -display none -d in_asm
