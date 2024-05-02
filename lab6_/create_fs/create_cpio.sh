#!/bin/sh

# Go to rootfs folder
# Select all files and put into archives
# '-o, --create' Run in copy-out mode
# '-H FORMAT' archive format, 
#    newc: SVR4 portable format

cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
