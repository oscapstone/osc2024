.PHONY: all bootloader kernel run-kernel run-bootloader clean

all: bootloader kernel

bootloader:
	$(MAKE) -C bootloader LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

kernel:
	$(MAKE) -C kernel LIB_PATH=$(CURDIR)/lib BSP_PATH=$(CURDIR)/bsp

run-kernel:
	$(MAKE) -C kernel run

run-bootloader:
	$(MAKE) -C bootloader run

clean:
	$(MAKE) -C bootloader clean
	$(MAKE) -C kernel clean
