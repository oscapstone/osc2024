#!/usr/bin/env python3
import struct, sys

SERIAL = sys.argv[1]

with open("./disk/kernel.img", "rb") as f:
    kernel = f.read()
    size = len(kernel)

with open(SERIAL, "wb", buffering=0) as tty:
    tty.write(b"s")
    tty.write(struct.pack("<I", size))
    tty.write(kernel)
    tty.write(b"j")
