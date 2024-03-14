#!/usr/bin/env python3
import serial
import argparse
import time

parser = argparse.ArgumentParser(description='OSDI 2023')
parser.add_argument('--filename', metavar='PATH',
                    default='./kernel/kernel8.img', type=str, help='path to script')
parser.add_argument('--device', metavar='TTY',
                    default='/dev/ttyUSB0', type=str,  help='path to UART device')
parser.add_argument('--baud', metavar='Hz', default=115200,
                    type=int,  help='baud rate')

args = parser.parse_args()

with open(args.filename, 'rb') as file:
    with serial.Serial(args.device, args.baud) as device:

        raw = file.read()
        length = len(raw)
        length_sent = len(raw)
        print("image size: {}".format(length))

        while length_sent > 0:

            digit = length_sent % 10
            length_sent = length_sent // 10

            device.write(str(digit).encode())
            device.flush()

        device.write(str('\n').encode())
        device.flush()

        for i in range(length):
            device.write(raw[i: i+1])
            device.flush()
            #print(i)
        print("finish")
    time.sleep(5)
    print("done")
