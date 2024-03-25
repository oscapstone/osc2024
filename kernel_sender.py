'''
Sent data to uart port to ttyUSB0
Usage: python3 kernel_sender.py device kernel_image

'''

import sys
import threading

def reader(path):
    ...
    
    

if __name__ == "__main__":
    # Open kernel image file
    if len(sys.argv) != 3:
        print("Usage: python3 kernel_sender.py device kernel_image")
        sys.exit(1)
    with open(sys.argv[2], "rb") as kernel:
        kernel_data = kernel.read()
        # Print the size of the kernel image
        print("Kernel size: ", len(kernel_data))
        # Open the ttyUSB0 port
        # New thread to read the response from ttyUSB0
        # reader_thread = threading.Thread(target=reader, args=(sys.argv[1],))
        # reader_thread.start()
        with open(sys.argv[1], "wb", buffering = 0) as tty:
            # Send kernel size to ttyUSB0
            tty.write(len(kernel_data).to_bytes(4, byteorder='little'))
            # Write the kernel image to the ttyUSB0 port
            tty.write(kernel_data)
            print("kernel_sender: Kernel image sent to ttyUSB0")            
            # Close the ttyUSB0 port
            tty.close()
    with open(sys.argv[1], "rb", buffering = 0) as tty:
        # read a char from ttyUSB0 and print it
        while True:
            response = tty.read(1)
            response = response.decode("ASCII")
            print(response, end="")
        tty.close()
