#!/usr/bin/env python3
import struct, sys, time

SERIAL = sys.argv[1]

with open("./disk/kernel.img", "rb") as f:
    kernel = f.read()
    size = len(kernel)

with open(SERIAL, "wb", buffering=0) as tty:
    print("reboot rpi")
    tty.write(b"reboot\n")
    time.sleep(2)

    print("send command")
    tty.write(b"s")
    time.sleep(0.1)

    print("send kernel size")
    tty.write(struct.pack("<I", size))
    time.sleep(0.1)

    print("send kernel")
    tty.write(kernel)
    time.sleep(1)

    print("jump kernel")
    tty.write(b"j")
