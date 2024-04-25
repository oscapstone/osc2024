#!/usr/bin/env python3

from serial import Serial
from pwn import *
import argparse

parser = argparse.ArgumentParser(description='Kernel Sender')
parser.add_argument('--filename', metavar='PATH', default='kernel8.img', type=str, help='path to kernel8.img')
parser.add_argument('--device', metavar='TTY',default='/dev/tty.usbserial-0001', type=str,  help='path to UART device')
parser.add_argument('--baud', metavar='Hz',default=115200, type=int,  help='baud rate')
args = parser.parse_args()

with open(args.filename,'rb') as fd:
    with Serial(args.device, args.baud) as ser:

        kernel_raw = fd.read()
        length = len(kernel_raw)

        print("Kernel image size : ", hex(length))
        for i in range(8):
            ser.write(p64(length)[i:i+1])
            ser.flush()

        print("Start sending kernel by uart1...")
        for i in range(length):
            # Use kernel_raw[i: i+1] is byte type. 
            # Using kernel_raw[i] it will retrieve int type then cause error
            ser.write(kernel_raw[i: i+1])
            ser.flush()
            if i % 100 == 0:
                print("{:>6}/{:>6} bytes".format(i, length))
        print("{:>6}/{:>6} bytes".format(length, length))
        print("Done!")