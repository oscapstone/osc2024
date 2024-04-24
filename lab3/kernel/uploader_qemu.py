import serial
import os
import os.path
import sys

def read_line(s):
    received_string = ""
    for i in range(13):
        c = s.read().decode()
        print(c)
        received_string += c
    return received_string

if __name__ == '__main__':



    if len(sys.argv) >= 2:
        deviceName = sys.argv[1]
    else:
        deviceName = "/dev/pts/2"

    s = serial.Serial(deviceName, baudrate=115200)

    kernel_filePath = "kernel.img"

    while (not s.writable()):
        pass

    size = os.stat(kernel_filePath).st_size 
    size_bytes = size.to_bytes(4, 'little')
    print('Sending size...', size)
    for i in range(4):
        print(size_bytes[i])
    s.write(size_bytes)
    print('size send...')

    print('Reciving size result...')
    sizeData = bytes()
    for i in range(4):
        dat = s.read(1)
        sizeData += dat

    receiveSize = int.from_bytes(sizeData, 'little')
    print('Return size: ', receiveSize)

    if (receiveSize != size):
        print('Not correct size for sending')
        print('Return size: ', receiveSize)
        exit(-1)
    print('Correct!')

    print('Sending data...')
    with open(kernel_filePath,"rb") as f:
        fileContent = f.read()
        print('0x46c: ', hex(fileContent[1132]), hex(fileContent[1133]), hex(fileContent[1134]), hex(fileContent[1135]))
        s.write(fileContent)

    print('data send')
    print('receiving result...')
    received_content = read_line(s)
    if received_content != "kernel loaded":
        print('loaded failed.')
        exit(-1)

    print('success!')

