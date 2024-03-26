import argparse
import serial
import os
import time
import numpy as np
from tqdm import tqdm

parser = argparse.ArgumentParser()
parser.add_argument("--image", type=str, default="kernel8.img")
parser.add_argument("--tty", type=str, default="/dev/ttyUSB0")
args = parser.parse_args()


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

    ser.write(file_size.to_bytes(4, byteorder="big"))
    time.sleep(0.01)

    print(f"Image Size: {file_size}")

    for i in tqdm(range(file_size), desc="transmission:"):
        ser.write(bytecodes[i].to_bytes())
        while not ser.writable():
            print("not writable")
            pass

    print("\nData transmission complete.")
    ser.close()


if __name__ == "__main__":
    main()
