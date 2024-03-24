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

def read_line(s):
    received_string = ""
    while True:
        c = s.read().decode()
        if c == "\r":
            continue
        if c == "\n":
            break
        received_string += c
    return received_string

# Check if file exists
if not os.path.exists(args.file_path):
    print(f"File {args.file_path} not found.")
    exit(1)

# Get file size and send it
size = os.stat(args.file_path).st_size
size_bytes = size.to_bytes(4, "little")
s.write(size_bytes)
print("wrote data")
time.sleep(3)
# Read and print the received size
#received_size = read_line(s)
#print(received_size)
print("sending kernel")

# Open and send the file
with open(args.file_path, "rb") as f:
    s.write(f.read())

# Read and print the received content
#received_content = read_line(s)
#print(received_content)
