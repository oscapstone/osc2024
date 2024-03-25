import os
import os.path
import argparse
import time

# Set up argument parser
parser = argparse.ArgumentParser(description='Send a file over serial connection.')
parser.add_argument('-s', '--serial_path', default='/dev/ttyUSB0', help='The path to the serial device (default: /dev/ttyUSB0)')
parser.add_argument('-f', '--file_path', default='kernel8.img', help='The file to send (default: kernel8.img)')
args = parser.parse_args()

# Check if file exists
if not os.path.exists(args.file_path):
    print(f"File {args.file_path} not found.")
    exit(1)

# Get file size
size = os.stat(args.file_path).st_size
size_bytes = size.to_bytes(4, "little")

# Open serial connection
with open(args.serial_path, "wb", buffering=0) as tty:
    # Send file size
    tty.write(size_bytes)
    print("wrote data")
    time.sleep(3)
    print("sending kernel")

    # Open and send the file
    with open(args.file_path, "rb") as f:
        tty.write(f.read())
