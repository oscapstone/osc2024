#!/usr/bin/env python3

# Import necessary libraries
from serial import Serial
import struct
import argparse
from sys import platform
import time

# Check if the platform is Linux and set the parser arguments accordingly
if platform == "linux" or platform == "linux2":
    # Create an argument parser for command line options
    parser = argparse.ArgumentParser(description='NYCU OSDI kernel sender')
    # Add command line arguments for the kernel image path, UART device, and baud rate
    parser.add_argument('--filename', metavar='PATH', default='../kernel/kernel8.img', type=str, help='path to kernel8.img')
    parser.add_argument('--device', metavar='TTY',default='/dev/ttyUSB0', type=str,  help='path to UART device')
    parser.add_argument('--baud', metavar='Hz',default=115200, type=int,  help='baud rate')
    # Parse the command line arguments
    args = parser.parse_args()

    # Open the kernel image file and UART device
    with open(args.filename,'rb') as fd:
        with Serial(args.device, args.baud) as ser:

            # Read the kernel image into memory
            kernel_raw = fd.read()
            # Get the length of the kernel image
            length = len(kernel_raw)

            # Print the size of the kernel image
            print("Kernel image size : ", hex(length))
            
            # convert the length of the kernel image to a series of bytes in a little-endian order (which is a standard for x86 and x86_64 architectures)
            for i in range(8):
                ser.write(struct.pack('<Q', length)[i:i+1]) # The slicing ensures that each iteration sends exactly one byte
                ser.flush()

            # Begin sending the kernel image over UART
            start_time = time.time()
            print("Start sending kernel image by uart1...")
            # Send each byte of the kernel image
            for i in range(length):
                # Write each byte to the serial port and flush immediately
                ser.write(kernel_raw[i: i+1])
                ser.flush()
                # Print progress every 100 bytes
                if i % 100 == 0:
                    print("{:>6}/{:>6} bytes".format(i, length)) # the value should be right-aligned within a field of width 6 characters. If the value has fewer than 6 characters, it will be padded with spaces on the left side.
            # Print final transfer size
            print("{:>6}/{:>6} bytes".format(length, length))
            end_time = time.time()
            # Indicate that the transfer is complete
            print("Transfer finished!")
            print(f"Total transfer time: {end_time - start_time:.2f} seconds")

# If the platform is not Linux, set up the parser for Windows or other platforms
else:
    # The setup for non-Linux platforms is almost identical to Linux, with the difference
    # being the default value for the `--device` argument, which uses 'COM#' format.
    # The rest of the script is the same as the Linux version and is omitted for brevity.
    parser = argparse.ArgumentParser(description='NYCU OSDI kernel sender')
    parser.add_argument('--filename', metavar='PATH', default='../kernel/kernel8.img', type=str, help='path to kernel8.img')
    parser.add_argument('--device', metavar='COM',default='COM3', type=str,  help='COM# to UART device')
    parser.add_argument('--baud', metavar='Hz',default=115200, type=int,  help='baud rate')
    args = parser.parse_args()

    with open(args.filename,'rb') as fd:
        with Serial(args.device, args.baud) as ser:

            kernel_raw = fd.read()
            length = len(kernel_raw)

            print("Kernel image size : ", hex(length))
            for i in range(8):
                ser.write(p64(length)[i:i+1])
                ser.flush()

            print("Start sending kernel image by uart1...")
            for i in range(length):
                # Use kernel_raw[i: i+1] is byte type. Instead of using kernel_raw[i] it will retrieve int type then cause error
                ser.write(kernel_raw[i: i+1])
                ser.flush()
                if i % 100 == 0:
                    print("{:>6}/{:>6} bytes".format(i, length))
            print("{:>6}/{:>6} bytes".format(length, length))
            print("Transfer finished!")
