import os
import argparse
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument('--file', '-f', type=str, default='../kernel8.img')
parser.add_argument('--device', '-d', type=str, default='/dev/ttyUSB0')
args = parser.parse_args()

fsize = os.path.getsize(args.file)
print((str(fsize)).encode())

flag = 0

with open(args.file, "rb", buffering=0) as f:
    with serial.Serial(args.device, 115200) as tty:
        tty.write((str(fsize)+'\0').encode())
        time.sleep(1)

        for i in range(fsize):
            flag = flag + 1
            fi = f.read(1)
            tty.write(fi)
            time.sleep(0.001)
        print(flag)
        print("send_img.py done")