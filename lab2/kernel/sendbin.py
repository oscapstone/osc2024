import serial
import os
import os.path
import sys
import time

s = serial.Serial("/dev/tty.usbserial-0001",baudrate=115200)

size = os.stat("kernel8.img").st_size 
size_bytes = size.to_bytes(4,"little")
s.write(size_bytes)

time.sleep(1) # flush function

with open("kernel8.img","rb") as f:
    # s.write(f.read())
    while True:
        data = f.read(1)
        if not data:
            break
        s.write(data)



