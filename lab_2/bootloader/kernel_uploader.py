import serial
import os
import os.path
import sys
import time

port = '/dev/pts/14'
s = serial.Serial(port, baudrate=115200)

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

size = os.stat("../shell/shell_kernel.img").st_size
# size = os.stat("./kernel8.img").st_size

size_bytes = size.to_bytes(4,"little")
print(f"size from python: {hex(size)}")
s.write(size_bytes)

# get response of correctness of bytes size sent
received_size = read_line(s)
print(received_size)

with open("../shell/shell_kernel.img","rb") as f:
    s.write(f.read())

# with open("./kernel8.img","rb") as f:
#     s.write(f.read())

# get response of whether kernel is loaded successfully or not
time.sleep(1)
while(True):
    while s.in_waiting > 0:
        received_content = read_line(s)
        print(received_content)
    inp = input()
    if(inp == ""):
        continue
    inp += "\n"
    bytes_to_send = inp.encode('utf-8')
    s.write(bytes_to_send)
    while s.in_waiting > 0:
        received_content = read_line(s)
        print(received_content)
    time.sleep(2)