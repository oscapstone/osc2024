#!/usr/bin/env python3
import struct, sys, time

SERIAL = sys.argv[1]

with open("./disk/kernel.img", "rb") as f:
    kernel = f.read()
    size = len(kernel)

with open(SERIAL, "wb", buffering=0) as tty:
    tty.write(b"s")
    time.sleep(1)
    tty.write(struct.pack("<I", size))
    time.sleep(1)
    tty.write(kernel)
    time.sleep(1)
    tty.write(b"j")
