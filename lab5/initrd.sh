#!/bin/bash

# This script creates a cpio archive for the initrd

cd rootfs
find . | cpio -o -H newc > ../initramfs.cpio
cd ..