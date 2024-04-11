import serial
import os
import os.path
import sys

# s = serial.Serial("/dev/ttyUSB0",baudrate=115200)
s = serial.Serial("/dev/ttyUSB0",baudrate=115200)

def read_line(s):
    received_string = ""
    while True:
        c = s.read().decode()
        if c=="\r":
            continue
        if c=="\n":
            break
        received_string += c
    return received_string

size = os.stat("shell_kernel.img").st_size
size_bytes = size.to_bytes(4,"little")
print(size)
print(size_bytes)
# t = 'a'
# s.write(t.encode())
s.write(size_bytes)

received_size = read_line(s)
