BASEURL 	= https://raw.githubusercontent.com/raspberrypi/firmware/master/boot
DISKNAME 	= RPI-BOOT
DISKPATH 	= /Volumes/$(DISKNAME)

FIRMWARE 	= bootcode.bin fixup.dat start.elf
CONFIG 		= config.txt
IMGS 		= $(shell find . -name '*.img')

.PHONY: all upload eject format download

all: $(FIRMWARE)

upload: $(FIRMWARE)
	cp $(FIRMWARE) $(DISKPATH)
ifneq ($(wildcard $(CONFIG)),)
	cp $(CONFIG) $(DISKPATH)
endif
ifneq ($(IMGS),)
	cp $(IMGS) $(DISKPATH)
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
