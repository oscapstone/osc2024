import argparse
import serial
import os
import time
import sys
import numpy as np
from tqdm import tqdm

parser = argparse.ArgumentParser()
parser.add_argument("--image", type=str, default="kernel8.img")
parser.add_argument("--tty", type=str, default="/dev/ttyUSB0")
args = parser.parse_args()


def checksum(bytecodes):
    return int(np.array(list(bytecodes), dtype=np.int32).sum())


def main():
    try:
        ser = serial.Serial(args.tty, 115200)
    except:
        print("Serial init failed!")
        exit(1)

    file_path = args.image
    file_size = os.stat(file_path).st_size

    with open(file_path, "rb") as f:
        bytecodes = f.read()

    file_checksum = checksum(bytecodes)

    ser.write(file_size.to_bytes(4, byteorder="big"))
    time.sleep(0.01)
    ser.write(file_checksum.to_bytes(4, byteorder="big"))
    time.sleep(0.01)
    print(f"Image Size: {file_size}, Checksum: {file_checksum}")

    # per_chunk = 32
    # chunk_count = file_size // per_chunk
    # chunk_count = chunk_count + 1 if file_size % per_chunk else chunk_count
    # print(f"# of chunk: {chunk_count}, per_chunk: {per_chunk}")

    for i in tqdm(range(file_size), desc="transmission:"):
        ser.write(bytecodes[i].to_bytes())
        while not ser.writable():
            print("not writable")
            pass

    # for i in tqdm(range(chunk_count), desc="transmission:"):
    #     ser.write(bytecodes[i * per_chunk : (i + 1) * per_chunk])
    #     while not ser.writable():
    #         print("not writable")
    #         pass
    #     time.sleep(0.01)

    print("\nData transmission complete.")
    ser.close()


if __name__ == "__main__":
    main()
