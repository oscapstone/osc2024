import serial
import os
import os.path
import argparse
import time

# Set up argument parser
parser = argparse.ArgumentParser(description='Send a file over serial connection.')
parser.add_argument('-s', '--serial_path', default='/dev/ttyUSB0', help='The path to the serial device (default: /dev/ttyUSB0)')
parser.add_argument('-f', '--file_path', default='kernel8.img', help='The file to send (default: kernel8.img)')
args = parser.parse_args()

# Setup serial connection
s = serial.Serial(args.serial_path, baudrate=115200)

# Check if file exists
if not os.path.exists(args.file_path):
    print(f"File {args.file_path} not found.")
    exit(1)

# Get file size and send it
size = os.stat(args.file_path).st_size
# send file size for loading
size_bytes = size.to_bytes(4, "little") #little endian to fit the environment
s.write(size_bytes)
print("wrote data")
time.sleep(3) #sleep to wait for uart
print("sending kernel")

# send file (raw binary)
with open(args.file_path, "rb") as f:
    s.write(f.read())