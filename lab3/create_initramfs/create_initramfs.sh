#!/bin/bash

echo "Compiling program in ./user_program..."
cd user_program
if make; then
    echo "Compilation successful."
else
    echo "Compilation failed."
    exit 1
fi

if [ -f "example.img" ]; then
    echo "Copying example.img to rootfs directory..."
    cp example.img ../rootfs/
else
    echo "example.img does not exist."
    exit 1
fi
cd ..

if [ ! -d "rootfs" ]; then
    echo "Directory 'rootfs' does not exist."
    exit 1
fi

cd rootfs
echo "Creating initramfs.cpio from contents of rootfs..."
if find . | cpio -o -H newc > ../../initramfs.cpio; then
    echo "initramfs.cpio created successfully."
else
    echo "Failed to create initramfs.cpio."
    exit 1
fi

cd ..

echo "Script completed."
