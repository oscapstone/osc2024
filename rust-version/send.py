#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import subprocess
import time

# FIXME: use rust to rewrite this script maybe good for rust exercise

KERNEL_NAME = "kernel8.img"
DEVICE_NAME = "/dev/pts/15"
# DEVICE_NAME = "/dev/ttyUSB0"
DEVICE_BAUDRATE = "115200"

subprocess.run(["stty", "-F", DEVICE_NAME, DEVICE_BAUDRATE])

f_ker = open(KERNEL_NAME, "rb")
kernel_bin = f_ker.read()
kernel_bin_size = len(kernel_bin)
f_ker.close()

with open(DEVICE_NAME, "wb+", buffering = 0) as f:

    f.write(kernel_bin_size.to_bytes(4, byteorder='big'))

    for i in range(kernel_bin_size):
        f.write(kernel_bin[i:i+1])
        if i % 1024 == 0:
            time.sleep(.01)
    print("kernel sent ended")


    f.flush()

# start subprocess to attach to the serial port
# subprocess.run(["screen", DEVICE_NAME, "115200"])
subprocess.run(["busybox", "microcom", "-s", DEVICE_BAUDRATE, DEVICE_NAME])

