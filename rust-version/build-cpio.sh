#!/bin/bash
find rootfs | cpio -o -H newc > ./initramfs.cpio
