# usage
# $ python sendimg.py kernel8.img /dev/cu.usbserial-0001
# $ python sendimg.py kernel8.img /dev/ttys002 # test in qemu
# $ python sendimg.py kernel8.img /dev/pts/2 # test in qemu, ubuntu, not work!!

import argparse
import serial
import os
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument("image")
parser.add_argument("tty")
args = parser.parse_args()

try:
    ser = serial.Serial(args.tty, 115200)
except:
    print("Serial init failed!")
    exit(1)

def receiveMsg():
    if ser.inWaiting() > 0:
        # Read data from buffer
        data = ser.read(ser.inWaiting())
        print(data.decode(errors='ignore'))
    else:
        print('====No data available====')

def sendMsg():
    cmd = input("$ ")
    if cmd == "exit":
        exit()
    cmd = cmd + '\n'
    for c in cmd:
        ser.write(c.encode())
        time.sleep(0.01)
    # ser.write(b'\n')
    time.sleep(0.1)
    return 1

def communicate():
    while True:
        receiveMsg()
        if sendMsg():
            time.sleep(0.01)
            receiveMsg()
            time.sleep(0.01)
        else:
            break

def main():
    file_path = args.image
    file_size = os.stat(file_path).st_size
    print(file_size)

    with open(file_path, 'rb') as f:
        bytecodes = f.read()

    # send command to Raspi 3
    time.sleep(0.1)
    cmd = "loadimg\n"
    for c in cmd:
        ser.write(c.encode())
        time.sleep(0.01)
    time.sleep(0.1)

    # Write file size to Raspi 3
    ser.write(file_size.to_bytes(4, byteorder="big"))
    time.sleep(0.01)

    # Show the message printed by Raspi 3
    receiveMsg()

    per_chunk = 64
    # Compute the chunk count of image file
    chunk_count = file_size // per_chunk
    chunk_count = chunk_count + 1 if file_size % per_chunk else chunk_count

    for i in range(chunk_count):
        sys.stdout.write('\r')
        sys.stdout.write("%d/%d" % (i + 1, chunk_count))
        sys.stdout.flush()
        ser.write(bytecodes[i * per_chunk: (i+1) * per_chunk])
        while not ser.writable():
            pass
    receiveMsg()
    print("------------------")
    print(f"After sending image to Raspi 3, Image Size: {hex(file_size)}")
    communicate()

if __name__ == "__main__":
    main()
