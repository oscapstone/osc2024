#!/bin/bash

# Check if sdcard.img exists
if [ ! -f "sdcard.img" ]; then
    echo "Error: sdcard.img not found!"
    exit 1
fi

# Mount sdcard.img to a temporary directory
mkdir -p ./sdcard_temp
sudo mount -o loop -t vfat sdcard.img ./sdcard_temp

# Copy files from mounted image to ./sdcard
sudo rsync -av ./sdcard_temp/ ./sdcard/

# Unmount the image
sudo umount ./sdcard_temp

# Clean up: remove temporary directory
rmdir ./sdcard_temp

echo "Extraction completed."
