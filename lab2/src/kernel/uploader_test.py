import serial
import os
import os.path
import sys

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

if __name__ == '__main__':

    s = serial.Serial("/dev/pts/6",baudrate=115200)

    kernel_filePath = "kernel.img"

    size = os.stat(kernel_filePath).st_size 
    size_bytes = size.to_bytes(4,"little")
    print('Sending size...', size)
    s.write(size_bytes)
    print('size send...')

    print('Reciving size result...')
    received_content = read_line(s)
    print(received_content)

    print('Sending data...')
    with open(kernel_filePath,"rb") as f:
        s.write(f.read())

    #received_content = read_line(s)
    #print(received_content)
