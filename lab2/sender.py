import serial
import os
import time

def sendKernelImg(s, f):
    tty = serial.Serial(s, 115200, timeout=0.1)
    file_stats = os.stat(f)
    # Send kernel size
    tty.write(str(file_stats.st_size).encode('utf-8'))
    tty.write('\n'.encode('utf-8')) # End by \n
    # Send kernel image byte-by-byte
    with open(f, "rb") as fp:
        byte = fp.read(1)
        while byte:
            tty.write(byte)
            byte = fp.read(1)
            time.sleep(0.0001)

if __name__ == '__main__':
    s = 'COM7'
    f = 'shell/kernel8.img'
    
    sendKernelImg(s, f)
