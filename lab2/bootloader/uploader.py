import serial
import os
import os.path
import sys
import time

s = serial.Serial("/dev/tty.usbserial-0001",baudrate=115200)

size = os.stat("shell_kernel.img").st_size 
size_bytes = size.to_bytes(4,"little")
s.write(size_bytes)

print("write size suceed:", size)
time.sleep(0.001)

with open("shell_kernel.img","rb") as f:
    s.write(f.read())
f.close()



