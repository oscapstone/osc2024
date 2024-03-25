all: bootloader kernel

DT=1
QEMU=0

.PHONY: bootloader kernel run send
bootloader:
	$(MAKE) -C bootloader
kernel:
	$(MAKE) -C kernel DT=$(DT) QEMU=$(QEMU)
run:
	$(MAKE) -C bootloader run
	
.PHONY: clean
clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C kernel clean
