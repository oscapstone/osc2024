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
echo t # Change partition type
echo c # Master Boot Record primary partitions type:LBA
echo w # Write changes
) | sudo fdisk $IMG_NAME

# 自動查找可用的循環設備並綁定文件，並將文件 myimage.img 綁定到該設備，同時顯示出使用的循環設備名稱。：
# sudo losetup --find --show myimage.img

# 啟用分區掃描，這會啟用對磁碟映像文件中分區的掃描，使得 /dev/loop0p1、/dev/loop0p2 等分區設備可用：
# sudo losetup --partscan --find --show myimage.img

LOOPBACK=`sudo losetup --partscan --show --find $IMG_NAME`

echo ${LOOPBACK} | grep --quiet "/dev/loop"

if [ $? = 1 ]
then
    echo "[!] losetup failed!"
    exit 1
fi

# 將新創建的分區格式化為FAT32：
sudo mkfs.vfat -F 32 ${LOOPBACK}p1

mkdir -p mnt

# 將分區掛載到掛載點：LFN
sudo mount -t vfat ${LOOPBACK}p1 mnt


# 將指定的文件複製到分區：
sudo cp -r sdcard/* mnt
# sudo cp ./bootloader.img mnt/bootloader.img
# sudo cp ./kernel8.img mnt/kernel8.img
sudo cp ./initramfs.cpio mnt/initramfs.cpio

# 卸載分區
sudo umount mnt

# 解除循環設備：
sudo losetup -d ${LOOPBACK}

# 將 ./sdcard.img 映像文件寫入到指定的 SD 卡設備
# dd if=./sdcard.img of=/dev/<your SD card device>
