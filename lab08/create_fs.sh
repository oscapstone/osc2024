#!/bin/bash

(cd initramfs && find . | cpio -o -H newc > ../initramfs.cpio)

