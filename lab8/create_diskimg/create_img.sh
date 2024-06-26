#!/bin/sh


IMG_NAME=sdcard.img

truncate -s 64M $IMG_NAME

(
echo o # Create a new empty DOS partition table
echo n # Add a new partition
echo p # Primary partition
echo 1 # Partition number
echo   # First sector (Accept default: 1)
echo   # Last sector (Accept default: varies)
echo a # Toggle bootable flag
echo t # Change partition type
echo b # W95 FAT32
echo w # Write changes
) | sudo fdisk $IMG_NAME

LOOPBACK=$(sudo losetup --partscan --show --find $IMG_NAME)

echo ${LOOPBACK} | grep --quiet "/dev/loop"

if [ $? = 1 ]; then
    echo "[!] losetup failed!"
    exit 1
fi

sudo mkfs.msdos -F 32 ${LOOPBACK}p1

mkdir -p mnt

sudo mount -t msdos ${LOOPBACK}p1 mnt

sudo cp ../kernel/kernel8.img mnt/KERNEL8.IMG
sudo cp ./initramfs.cpio mnt/INITRD
sudo cp ../config.txt mnt/CONFIT.TXT
sudo cp ../bcm2710-rpi-3-b-plus.dtb mnt/BCM2710.DTB
sudo cp ./FAT_R.TXT mnt/FAT_R.TXT
sudo bash -c "echo "Hello" > mnt/HELLO"
sudo bash -c 'echo -e "W\no\nr\nl\nd" > mnt/WORLD'

sudo umount -l mnt
sudo losetup -d ${LOOPBACK}
sudo rm -rf mnt

#mv sdcard.img ../ # 1fc400

# dd if=./sdcard.img of=/dev/<your SD card device>
