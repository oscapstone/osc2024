# usage
# $ python communicate.py /dev/cu.usbserial-0001
# $ python communicate.py /dev/ttys002 # test in qemu

import argparse
import serial
import os
import sys
import time
import fcntl

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
        print(data.decode(errors='ignore'), end='')

def communicate():
    fd = sys.stdin.fileno()
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)
    while True:
        try:
            key = sys.stdin.read(1)
        except BlockingIOError:
            pass
        else:
            if key == '\r':
                key = '\n' 
            ser.write(key.encode())
            time.sleep(0.01)
        receiveMsg()

def main():
    communicate()

if __name__ == "__main__":
    main()
