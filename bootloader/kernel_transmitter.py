#!/usr/bin/env python3
import argparse
from serial import Serial
from pwn import p64

parser = argparse.ArgumentParser(
    description="""
The NYCU OSC kernel sender is a tool designed to transmit kernel image files to embedded devices or development boards via the UART interface. 
This script supports several command-line options, allowing users to customize the file path, UART device path, and communication baud rate. 
It is extensively used in operating systems courses and embedded system development, especially during development phases that require frequent kernel image updates.
"""
)
parser.add_argument(
    "-f",
    "--file",
    metavar="PATH",
    default="./kernel8.img",
    type=str,
    help="Specifies the path to the kernel image file. The default path is 'kernel8.img'.",
)
parser.add_argument(
    "-d",
    "--device",
    metavar="TTY",
    default="/dev/cu.SLAB_USBtoUART",
    type=str,
    help="Specifies the path to the UART device file. The default is '/dev/cu.SLAB_USBtoUART'.",
)
parser.add_argument(
    "-b",
    "--baud",
    metavar="Hz",
    default=115200,
    type=int,
    help="Sets the baud rate for UART communication. The default is 115200.",
)
args = parser.parse_args()


with open(args.file, "rb") as fd:
    with Serial(args.device, args.baud) as ser:
        kernel_raw = fd.read()
        length = len(kernel_raw)

        print("Kernel image size: ", hex(length))
        for i in range(8):
            ser.write(p64(length)[i : i + 1])
            ser.flush()

        print("Start sending kernel image by uart1...")
        for i in range(0, length, 1024):
            chunk = kernel_raw[i : i + 1024]
            ser.write(chunk)
            ser.flush()
            print("{:>6}/{:>6} bytes".format(min(i + 1024, length), length), end="\r")

        print("{:>6}/{:>6} bytes".format(length, length))
        print("Transfer finished!")
