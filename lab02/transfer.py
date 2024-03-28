import argparse
import os, sys
import time
import serial

def IsDeviceExists(device_path):
    if os.path.exists(device_path):
        return True
    else:
        sys.stderr.write("Error: Device does not exist.\n")
        sys.exit(1)

def get_file_size(file_path):
    if os.path.exists(file_path):
        return os.path.getsize(file_path)
    else:
        sys.stderr.write("Error: File does not exist.\n")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Transfer the file to the device")
    parser.add_argument("device_path", type=str, help="Path to device. (e.g. /dev/ttyUSB0)")
    parser.add_argument("file_path", type=str, help="Path to the file")
    args = parser.parse_args()

    device_path = args.device_path
    file_path = args.file_path

    IsDeviceExists(device_path)
    kernel_size = get_file_size(file_path)
    size_in_bytes = (kernel_size).to_bytes(4, byteorder='big')
    
    print(f"Size of {file_path}: {size_in_bytes}")
    ser = serial.Serial (device_path, 115200)
    # with open(device_path, 'wb+', buffering=0) as tty, open(file_path, 'rb') as kf:
    input('Press any key to continue')
    with open(file_path, 'rb') as kf:
        # Send kernel size to device
        
        ser.write(size_in_bytes)
        print("Size sent")
        # time.sleep(1)
        # Send kernel binary
        
        kernel_byte = kf.read(1)
        for i in range(kernel_size):
            print(f"Sending {i}th byte")
            ser.write(kernel_byte)
            kernel_byte = kf.read(1)
            time.sleep(0.001)
        
        print("Kernel sent")

if __name__ == "__main__":
    main()
