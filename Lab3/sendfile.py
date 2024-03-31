import os
import sys
import time

FILENAME = "./kernel/kernel8.img"
BLOCK_SIZE = 1024

def get_file_size(filename):
    return os.stat(filename).st_size


def send_by_block(src_file, dst_file):
    
    
    size = get_file_size(FILENAME)

    dst_file.write(size.to_bytes(4, "big"))

    time.sleep(1)

    while (block := src_file.read(BLOCK_SIZE)):
        dst_file.write(block)
        print("Done")
        time.sleep(5)

def send_by_byte(src_file, dst_file):

    raw_data = src_file.read()
    size = len(raw_data)

    dst_file.write(size.to_bytes(4, "big"))
    time.sleep(1)

    for i in range(size):
        dst_file.write(raw_data[i].to_bytes(1, "big"))
        time.sleep(0.002)
        
        if (i+1)%1024 == 0:
            print(i)

        

if __name__ == '__main__':
    with open('/dev/ttyUSB0', "wb", buffering=0) as tty:
    # with open('/dev/pts/3', "wb", buffering=0) as tty:
        with open(FILENAME, 'rb') as file:
            send_by_block(file, tty)
        