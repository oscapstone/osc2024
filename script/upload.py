#!/usr/bin/env python3
import struct, sys, time

SERIAL = sys.argv[1]

with open("./disk/kernel.img", "rb") as f:
    kernel = f.read()
    size = len(kernel)

with open(SERIAL, "wb", buffering=0) as tty:
    print("reboot rpi")
    tty.write(b"reboot\n")
    time.sleep(3)

    print("send command")
    tty.write(b"s")
    time.sleep(0.5)

    print("send kernel size")
    tty.write(struct.pack("<I", size))
    time.sleep(0.5)

    print("send kernel", len(kernel))
    tty.write(kernel)

    print("jump kernel")
    tty.write(b"j")
