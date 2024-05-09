#!/usr/bin/env python3

import sys
import serial
import time
from pathlib import Path

def send_file(filename, serial_port):
    ser = None
    try:
        # Open the serial port
        ser = serial.Serial(serial_port, baudrate=115200, timeout=1)
        print(f"Opened serial port {serial_port}")

        # Read the file content
        with open(filename, 'rb') as file:
            file_content = file.read()
        print(f"Opened file {filename}")

        # Send the size of the file as 32-bit little-endian
        file_size = len(file_content)
        ser.write(file_size.to_bytes(4, 'little'))

        # Send the file content
        byte_counter = 0
        checksum = 0
        for chunk in chunks(file_content, file_size, 1):
            
            ser.write(chunk)
            
            checksum = (checksum % 100+ int.from_bytes(chunk, "little") % 100 ) % 100
            # time.sleep(0.01)
            # print(f"pos:{byte_counter} data:{int.from_bytes(chunk, "little")} checksum:{checksum}");

            byte_counter += len(chunk)
            
            if byte_counter % 256 == 0:
                time.sleep(0.02)  # Add a delay every 256 bytes
                print(f"uploading: {byte_counter}/{len(file_content)}")
                # print(f"context: {list(chunk)}")
                # print(f"context: {int.from_bytes(chunk, "little") }")
                print(f"checksum: {checksum}")

        # overofbound
        # checksum = 0
        # for i in range(file_size):
        #     ser.write(file_content[i])
        #     byte_counter += 1;
        #     checksum = (checksum + file_content[i] ) % 1024
            
        #     if byte_counter % 1024 == 0:
        #       print(f"context: {file_content[i]}")
        #       print(f"uploading: {byte_counter}/{file_size}");
        #       print(f"checksum: {checksum}")
        #       time.sleep(0.05)  # Add a delay every 1024 bytes

        
        print(f"uploading: {byte_counter}/{len(file_content)}");
        print(f"checksum: {checksum}")
        print("File sent successfully.")

        text = ser.readline()
        print(text)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if ser is not None and ser.is_open:  
            ser.close()
            print("Serial port closed.")

def chunks(lst, size ,n):
    """Yield successive n-sized chunks from lst."""
    for i in range(0, len(lst), n):
        # print(f"chunk start form {i} to {i+n}")
        if i+n > size:
            # print("END________")
            yield lst[i:size]
        else:
            yield lst[i:i + n]

def main():
    if len(sys.argv) != 3:
        print("Usage: python sender.py <filename> <serial_port>")
        sys.exit(1)

    filename = sys.argv[1]
    if not Path(filename).is_file():
        print(f"Error: File '{filename}' not found.")
        sys.exit(1)

    serial_port = sys.argv[2]

    send_file(filename, serial_port)

if __name__ == "__main__":
    main()
