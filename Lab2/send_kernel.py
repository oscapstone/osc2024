import time
import serial
import sys
import os
import struct

baud_rate = 115200            # Baud rate for the serial connection
timeout = 5                   # Timeout for serial read operations in seconds
kernel_img_path = './kernel8.img'

def send_file(serial_port, file_path, baud_rate, timeout):
    # Check if the file exists
    if not os.path.exists(file_path):
        print(f"File {file_path} does not exist.")
        return

    try:
        # Open the serial port
        with serial.Serial(serial_port, baud_rate, timeout=timeout) as ser:
            print(f"Opened serial port {serial_port} at {baud_rate} baud.")

            # Get the size of the file
            file_size = os.path.getsize(file_path)
            print(f"File size: {file_size} bytes.")

            #call bootloader to start
            
            #time.sleep(2)
            # Wait for Raspberry Pi
            ser.write((77).to_bytes(1, byteorder='little'))
            while(1):
                rasp_response = ser.readline().decode()
                print("Raspberry Pi :",repr(rasp_response))
                if rasp_response == "ready for receiving kernel size.\n":
                    break

            print("start sending kernel size.")
            # Send the file size as a 4-byte integer in little-endian format
            ser.write(file_size.to_bytes(4, byteorder='little'))
            #time.sleep(10)
            # Wait for Raspberry Pi
            
            #time.sleep(2)
            ser.write((77).to_bytes(1, byteorder='little'))
            while(1):
                rasp_response = ser.readline().decode()
                print("Raspberry Pi :",repr(rasp_response))
                
                if rasp_response == "ready for receiving kernel.\n":
                    break

            # Read the file and send it over the serial connection
            print("start sending kernel.")
            with open(file_path, 'rb') as f:
                ser.write(f.read())

            print(f"Successfully sent {file_path} to {serial_port}.")
            ser.close() 

    except serial.SerialException as e:
        print(f"Error opening or using serial port {serial_port}: {e}")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == '__main__':
    if len(sys.argv)>1:
        serial_port  = f"/dev/pts/{sys.argv[1]}"
    else:
        serial_port  = "/dev/ttyUSB0"

    send_file(serial_port, kernel_img_path, baud_rate, timeout)
    
