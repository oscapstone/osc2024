import argparse
import serial
import os
import time

parser = argparse.ArgumentParser(description="Send kernel image to bootloader")

parser.add_argument(
    "-p",
    "--port",
    type=str,
    default="/dev/tty.usbserial-0001",
    help="Serial port to send the kernel image",
)

parser.add_argument(
    "-f",
    "--file",
    type=str,
    default="../kernel8.img",
    help="Path to the kernel image file",
)

args = parser.parse_args()

# open serial port
tty = serial.Serial(args.port, 115200, timeout=0.5)
file_stats = os.stat(args.file)

# send file size
endline = "\n"
tty.write(str(file_stats.st_size).encode("utf-8"))
tty.write(endline.encode("utf-8"))

time.sleep(0.5)

size = file_stats.st_size
print(size)

# send file content byte by byte
with open(args.file, "rb") as fp:
    data = fp.read()
    tty.write(data)
    # byte = fp.read(1)
    # while byte:
    #     tty.write(byte)
    #     size -= 1
    #     byte = fp.read(1)
    #     time.sleep(0.0005)  # delay to avoid buffer overflow

print("File sent successfully")
print(size)