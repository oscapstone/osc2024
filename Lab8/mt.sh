#!/bin/bash

IMAGE="sfn_nctuos.img"
MOUNT_POINT="/mnt/fat32img"

START_SECTOR=2048
SECTOR_SIZE=512
OFFSET=$(($START_SECTOR * $SECTOR_SIZE))

if [ ! -d "$MOUNT_POINT" ]; then
    echo "Creating mount point..."
    sudo mkdir -p $MOUNT_POINT
fi

echo "Mounting image..."
sudo mount -o loop,offset=$OFFSET $IMAGE $MOUNT_POINT

echo "Listing files in the mounted image..."
ls -l $MOUNT_POINT

echo "To unmount the image, run: sudo umount $MOUNT_POINT"
