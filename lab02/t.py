#!/usr/bin/env python3

import sys

import time

devname = input("Enter a device name: ")
#fname = input("Enter the kernel filename: ")

#devname="/dev/ttyUSB0"
fname="../lab01/size_"

file = open(fname, "rb")

term = open(devname, "wb+", buffering=0)




for i in range(4):

    kern_size = file.read(1)
    time.sleep(0.001)

    term.write(kern_size)

file.close()
time.sleep(0.1)

fname="../lab01/kernel8.img"
#fname="/dev/pts/1"

file = open(fname, "rb")

kernbyte = file.read(1)
i=0
while kernbyte != b"":
    print(i)
    i=i+1
    time.sleep(0.001)
    term.write(kernbyte)
    kernbyte = file.read(1)

sys.stdout.write("kernel is sent\n")
sys.exit(0)
