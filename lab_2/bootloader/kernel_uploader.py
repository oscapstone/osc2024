import serial
import os
import os.path
import sys

port = '/dev/pts/2'
s = serial.Serial(port, baudrate=115200)

def read_line(s):
    received_string = ""
    while True:
        c = s.read().decode()
        if c=="\r":
            break
        if c=="\n":
            break
        received_string += c
    return received_string

print("S")
size = os.stat("../shell/shell_kernel.img").st_size
received_size = read_line(s)
print(received_size)
size_bytes = size.to_bytes(4,"little")
s.write(size_bytes)

received_size = read_line(s)
print(received_size)

with open("../shell/shell_kernel.img","rb") as f:
    s.write(f.read())

received_content = read_line(s)
print(received_content)
print("reloading")
received_content = read_line(s)
print(received_content)