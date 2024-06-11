BASEURL 	= https://raw.githubusercontent.com/raspberrypi/firmware/master/boot
DISKNAME 	= RPI-BOOT
DISKPATH 	= /Volumes/$(DISKNAME)

FIRMWARE 	= bootcode.bin fixup.dat start.elf
CONFIG 		= config.txt
BOOTLOADER 	= bootloader.img
INITFSCPIO 	= initramfs.cpio
DEVICETREE 	= bcm2710-rpi-3-b-plus.dtb
FAT32_TEST 	= FAT_R.TXT
IMAGES 		= nycuos.img sfn_nctuos.img

.PHONY: all files upload eject format download dtb images

all: files

files: $(FIRMWARE) $(DEVICETREE) $(FAT32_TEST)

upload: files
	cp $(FIRMWARE) $(DEVICETREE) $(BOOTLOADER) $(DISKPATH)
ifneq ($(wildcard $(CONFIG)),)
	cp $(CONFIG) $(DISKPATH)
endif
ifneq ($(wildcard $(INITFSCPIO)),)
	cp $(INITFSCPIO) $(DISKPATH)
endif
	cp $(FAT32_TEST) $(DISKPATH)

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

FAT_R.TXT:
	wget https://nycu-caslab.github.io/OSC2024/_downloads/10fbdc3e04b471849e714edcdcf4ce26/FAT_R.TXT

images: $(IMAGES)

nycuos.img:
	wget https://github.com/nycu-caslab/OSC2024/raw/main/supplement/nycuos.img

sfn_nctuos.img:
	wget https://github.com/GrassLab/osdi/raw/master/supplement/sfn_nctuos.img
