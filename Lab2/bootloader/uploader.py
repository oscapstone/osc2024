import serial
import os
import os.path
import sys

s = serial.Serial("COM3",baudrate=115200)

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

size = os.stat("kernel8_1.img").st_size 
size_bytes = size.to_bytes(4,"little")
s.write(size_bytes)

received_size = read_line(s)
print(received_size)

with open("kernel8_1.img","rb") as f:
    s.write(f.read())

received_content = read_line(s)
print(received_content)



