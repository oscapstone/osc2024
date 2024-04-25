ARMGNU ?= aarch64-linux-gnu

all: test_svc_exception.img

test_svc_exception.img: test_svc_exception.o
	$(ARMGNU)-ld -o test_svc_exception.elf test_svc_exception.o
	$(ARMGNU)-objcopy test_svc_exception.elf -O binary test_svc_exception.img
	cp ./test_svc_exception.img ../rootfs/
	(cd ../rootfs/ && find . | cpio -o -H newc > ../initramfs.cpio)

test_svc_exception.o: test_svc_exception.S
	$(ARMGNU)-gcc -MMD -c $< -o $@

clean:
	rm -f *.o *.d *.elf *.img

dump: test_svc_exception.img
	qemu-aarch64 -M raspi3 -kernel test_svc_exception.img -display none -d in_asm
