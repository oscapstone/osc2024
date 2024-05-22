#!/usr/bin/env python3

import serial
import os
import time
from argparse import ArgumentParser

def passImg(f, s):
    if (s == "/dev/ttyUSB0") or (s == "/dev/tty.usbserial-0001"):
        delay = 0.0003
    else:
        delay = 0.0003
    
    tty = serial.Serial(s, 115200, timeout=0.5) 
    file_stats = os.stat(f)

    #tty.write("boot\n".encode('utf-8'))
    time.sleep(delay)

    time.sleep(delay*100)
    tty.write(str(file_stats.st_size).encode('utf-8'))
    time.sleep(delay*100)

    time.sleep(delay*100)
    tty.write("\n".encode('utf-8'))
    time.sleep(delay*100)

    print('File size: {} bytes'.format(file_stats.st_size))
    cnt = 0
    
    with open(f, "rb") as fp:
        byte = fp.read(1)
        while byte:
            tty.write(byte)
            byte = fp.read(1)
            if cnt % 1000 == 0:
                print('{:6} bytes sent\t{:.5f}% completed'.format(cnt, (cnt / file_stats.st_size) * 100))
            cnt += 1
            time.sleep(delay)

    tty.close()

def main():
    parser = ArgumentParser()
    parser.add_argument("-i", help="kernel image of tty", default="./kernel8.img")
    parser.add_argument("-s", help="send kernel image to which serial console", default="/dev/tty.usbserial-0001")
    args = parser.parse_args()

    passImg(args.i, args.s)

if __name__ == "__main__":
    main()