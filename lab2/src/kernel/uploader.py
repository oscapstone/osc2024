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

    s = serial.Serial("COM3",baudrate=115200)

    kernel_filePath = "kernel.img"

    size = os.stat(kernel_filePath).st_size 
    size_bytes = size.to_bytes(4,"little")
    print('Sending size...', size)
    s.write(size_bytes)
    print('size send...')

    print('Reciving size result...')
    sizeData = bytes()
    for i in range(4):
        sizeData += s.read(1)
    receiveSize = int.from_bytes(sizeData)

    if (receiveSize != size):
        print('Not correct size for sending')
        print('Return size: ', receiveSize)
        exit(-1)
    print('Correct!')

    print('Sending data...')
    with open(kernel_filePath,"rb") as f:
        s.write(f.read())

    received_content = read_line(s)
    print(received_content)
