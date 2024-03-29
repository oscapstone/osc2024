#!/bin/bash

# Compile user program
cd user_program
make clean && make
cp user.img ../rootfs/user.img
cd ..


# Produce initramfs.cpio
cd rootfs
pwd
find . | cpio -o -H newc > ../initramfs.cpio
cd ..
