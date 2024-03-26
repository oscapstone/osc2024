import serial
import os
import sys
import time

def read_kernel(s):
    size = os.stat("../kernel8.img").st_size
    print(f"kernel size: {size}")
    s.write(size.to_bytes(4, byteorder='little'))
    time.sleep(5)
    print(s.read_until(b"Loading...\r\n").decode())
    with open("../kernel8.img", "rb") as f:
        s.write(f.read())
    print(s.read_until(b"Kernel loaded!!!\r\n").decode())

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/pts/2'
    s = serial.Serial(port, baudrate=115200, timeout=5)
    read_kernel(s)
    s.close()