.PHONY: boot kernel copyboot runboot runkernel clean

boot:
	make -C ./bootloader

kernel:
	make -C ./kernel

copyboot:
	make copy -C ./bootloader

runboot: 
	make run -C ./bootloader

runkernel:
	make run -C ./kernel

clean:
	make clean -C ./bootloader
	make clean -C ./kernel