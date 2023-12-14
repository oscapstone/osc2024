#!/bin/bash

cd user_program
make clean && make
cp user.img ../rootfs
cd ..


# Compile user program to initramfs.cpio
cd rootfs
pwd
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
