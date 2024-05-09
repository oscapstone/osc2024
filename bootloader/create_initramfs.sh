#!/bin/bash

if [ ! -d "rootfs" ]; then
    echo "Directory 'rootfs' does not exist."
    exit 1
fi

cd rootfs

echo "Creating initramfs.cpio from contents of rootfs..."
find . | cpio -o -H newc > ../initramfs.cpio

if [ $? -eq 0 ]; then
    echo "initramfs.cpio created successfully."
else
    echo "Failed to create initramfs.cpio."
    exit 1
fi

cd ..

echo "Script completed."
