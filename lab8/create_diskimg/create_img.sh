#!/bin/sh

IMG_NAME=sdcard.img

# 1. Create a 64MB disk image file
truncate -s 64M $IMG_NAME

# 2. Create a new DOS partition table and partition
(
echo o # Create a new empty DOS partition table
echo n # Add a new partition
echo p # Primary partition
echo 1 # Partition number
echo   # First sector (Accept default: 1)
echo   # Last sector (Accept default: varies)
echo t # Change partition type
echo c # W95 FAT32 (LBA)
echo w # Write changes
) | sudo fdisk $IMG_NAME

# 3. Set up loop device and partition
sudo losetup -fP $IMG_NAME
LOOPBACK=$(sudo losetup -l | grep "$IMG_NAME" | awk '{print $1}')

# Check if loop device setup was successful
if [ -z "$LOOPBACK" ]; then
    echo "[!] losetup failed!"
    exit 1
fi

# 4. Format the new partition as FAT32 file system
sudo mkfs.vfat -F 32 ${LOOPBACK}p1

# 5. Mount the partition and copy files
mkdir -p mnt
sudo mount -t vfat ${LOOPBACK}p1 mnt

sudo cp -r sdcard/* mnt
sudo cp ./bootloader.img mnt/bootloader.img
sudo cp ./initramfs.cpio mnt/initramfs.cpio

# 6. Unmount the partition and detach the loop device
sudo umount mnt
sudo losetup -d ${LOOPBACK}
# dd if=./sdcard.img of=/dev/<your SD card device>
