#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import subprocess

# FIXME: use rust to rewrite this script maybe good for rust exercise

KERNEL_NAME = "kernel8.img"
f_ker = open(KERNEL_NAME, "rb")
kernel_bin = f_ker.read()
kernel_bin_size = len(kernel_bin)
f_ker.close()

with open("/dev/pts/3", "wb+", buffering = 0) as f:
    f.write(kernel_bin_size.to_bytes(4, byteorder='big'))

    line = f.readline().decode('utf-8')
    print(line, end='')
    if '[1]' in line:
        f.write(kernel_bin)
        print("kernel sent ended")

    line = f.readline().decode('utf-8')
    print(line, end='')

    # flush the buffer
    f.flush()

# start subprocess to attach to the serial port
subprocess.run(["screen", "/dev/pts/3", "115200"])
