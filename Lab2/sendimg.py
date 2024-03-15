import argparse
import serial

import os
import sys
import numpy as np


from serial.tools import list_ports
port = list(list_ports.comports())
print("Available Device:")
for p in port:
    print(p.device)
print("=================")

parser = argparse.ArgumentParser()
parser.add_argument("image")
parser.add_argument("tty")
args = parser.parse_args()

def checksum(bytecodes):
    # convert bytes to int
    return int(np.array(list(bytecodes), dtype=np.int32).sum())

def main():
    try:
        ser = serial.Serial(args.tty, 115200)
        print("Serial init success")
    except Exception as e:
        print("Serial init failed:", e)
        exit(1)

    file_path = args.image
    file_size = os.stat(file_path).st_size

    with open(file_path, 'rb') as f:
        bytecodes = f.read()
    # make checksum
    file_checksum = checksum(bytecodes)
    # write file_size
    ser.write(file_size.to_bytes(4, byteorder="big"))
    # write checksum
    ser.write(file_checksum.to_bytes(4, byteorder="big"))
    print(f"Image Size: {file_size}, Checksum: {file_checksum}")
    per_chunk = 128
    chunk_count = file_size // per_chunk
    chunk_count = chunk_count + 1 if file_size % per_chunk else chunk_count

    for i in range(chunk_count):
        sys.stdout.write('\r')
        sys.stdout.write("%d/%d" % (i + 1, chunk_count))
        sys.stdout.flush()
        # send chunk by chunk
        ser.write(bytecodes[i * per_chunk: (i+1) * per_chunk])     
        while not ser.writable():
            pass


if __name__ == "__main__":
    main()
