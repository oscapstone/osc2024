import serial
import time
import os

# Configuration
serial_port = '/dev/ttyUSB0'  # Adjust this to your system's serial port
# serial_port = '/dev/tty.PL2303-USBtoUART1120'  # Adjust this to your system's serial port
baud_rate = 115200            # Adjust baud rate as needed
kernel_image_path = 'build/kernel/kernel8.img'  # Path to your kernel image

# Function to send a file through serial
def send_file(file_path, port, baud):
    try:
        with serial.Serial(port, baud, timeout=1) as ser:
            # Get file size
            file_size = os.path.getsize(file_path)
            print(f"File size: {file_size} bytes")

            # Send file size (4 bytes, little-endian format)
            ser.write(file_size.to_bytes(4, byteorder='little'))
            time.sleep(0.5)  # Give the receiver a moment to process the size
            
            with open(file_path, 'rb') as file:
                print("Sending file...")
                while True:
                    chunk = file.read(1024)  # Read in chunks of 1024 bytes
                    if not chunk:
                        break  # End of file
                    ser.write(chunk)
                    time.sleep(0.02)  # Short delay to ensure data is transmitted properly
            print("File sent successfully.")
            
    except serial.SerialException as e:
        print(f"Error: {e}")
    except FileNotFoundError:
        print(f"Error: File {file_path} not found.")

# Main function
if __name__ == '__main__':
    send_file(kernel_image_path, serial_port, baud_rate)