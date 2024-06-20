#!/bin/sh

IMG_NAME=sd_sfn.img

truncate -s 64M $IMG_NAME

(
echo o # Create a new empty DOS partition table
echo n # Add a new partition
echo p # Primary partition
echo 1 # Partition number
echo   # First sector (Accept default: 1)
echo   # Last sector (Accept default: varies)
echo t # Change partition type
echo b # Master Boot Record primary partitions type:SFN
echo w # Write changes
) | sudo fdisk $IMG_NAME

LOOPBACK=`sudo losetup --partscan --show --find $IMG_NAME`

echo ${LOOPBACK} | grep --quiet "/dev/loop"

if [ $? = 1 ]
then
    echo "[!] losetup failed!"
    exit 1
fi

sudo mkfs.vfat -F 32 ${LOOPBACK}p1

mkdir -p mnt

# sudo mount -t vfat ${LOOPBACK}p1 mnt
sudo mount -t msdos ${LOOPBACK}p1 mnt # msdos for SFN?

sudo cp -r sdcard/* mnt

sudo umount mnt

sudo losetup -d ${LOOPBACK}
# dd if=./sdcard.img of=/dev/<your SD card device>