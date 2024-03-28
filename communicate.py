# usage
# $ python communicate.py /dev/cu.usbserial-0001
# $ python communicate.py /dev/ttys002 # test in qemu

import argparse
import serial
import os
import sys
import time

parser = argparse.ArgumentParser()
# parser.add_argument("image")
parser.add_argument("tty")
args = parser.parse_args()

try:
    ser = serial.Serial(args.tty, 115200)
except:
    print("Serial init failed!")
    exit(1)

def receiveMsg():
    if ser.inWaiting() > 0:
        # Read all data in buffer
        data = ser.read(ser.inWaiting())
        print(data.decode(errors='ignore'))
    else:
        print('====No data available====')

def sendMsg():
    cmd = input("$ ")
    if cmd == "exit":
        exit()
    cmd = cmd + '\n'
    for c in cmd:
        ser.write(c.encode())
        time.sleep(0.01)
    # ser.write(b'\n')
    time.sleep(0.1)
    return 1

def communicate():
    while True:
        receiveMsg()
        if sendMsg():
            time.sleep(0.01)
            receiveMsg()
            time.sleep(0.01)
        else:
            break

def main():
    communicate()

if __name__ == "__main__":
    main()
