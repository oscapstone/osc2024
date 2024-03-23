BASEURL 	= https://raw.githubusercontent.com/raspberrypi/firmware/master/boot
DISKNAME 	= RPI-BOOT
DISKPATH 	= /Volumes/$(DISKNAME)

FIRMWARE 	= bootcode.bin fixup.dat start.elf
CONFIG 		= config.txt
IMGS 		= $(shell find . -name '*.img')
INITFSCPIO 	= initramfs.cpio
DEVICETREE 	= bcm2710-rpi-3-b-plus.dtb

.PHONY: all upload eject format download dtb

all: $(FIRMWARE) $(DEVICETREE)

upload: $(FIRMWARE) $(DEVICETREE)
	cp $(FIRMWARE) $(DEVICETREE) $(DISKPATH)
ifneq ($(wildcard $(CONFIG)),)
	cp $(CONFIG) $(DISKPATH)
endif
ifneq ($(IMGS),)
	cp $(IMGS) $(DISKPATH)
endif
ifneq ($(wildcard $(INITFSCPIO)),)
	cp $(INITFSCPIO) $(DISKPATH)
endif

eject:
	diskutil eject $(DISKPATH)

format:
ifeq ($(DISK),)
	diskutil list
	@echo "No DISK selected :(" && false
else
	@echo "Are you sure format disk '$(DISK)'? [y/N]" && read ans && [ $${ans:-N} = y ]
	diskutil eraseDisk FAT32 $(DISKNAME) MBR $(DISK)
endif

download:
ifeq ($(TARGET),)
	@echo "No TARGET set :(" && false
else
	wget "$(BASEURL)/$(TARGET)" -O "$(TARGET)"
endif

bootcode.bin:
	$(MAKE) download "TARGET=$@"

fixup.dat:
	$(MAKE) download "TARGET=$@"

start.elf:
	$(MAKE) download "TARGET=$@"

dtb: $(DEVICETREE)
$(DEVICETREE):
	$(MAKE) download "TARGET=$@"
