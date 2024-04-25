import sys
import argparse
import serial

parser = argparse.ArgumentParser(description='Send kernel image to uart port')
parser.add_argument('--type', type=str, choices=['uart', 'pty'], help='Type of the device', nargs='?', default='uart')
parser.add_argument('device', type=str, help='Device file', nargs='?', default='/dev/ttyUSB0')
parser.add_argument('kernel_image', type=str, help='Kernel image file', nargs='?', default='kernel.img')

args = parser.parse_args()

if args.type == 'uart':
    print('UART device:', args.device)
    with open(args.kernel_image, "rb") as kernel:
        kernel_data = kernel.read()
        # Print the size of the kernel image
        print("Kernel size: ", len(kernel_data))
        with serial.Serial(args.device, 115200) as uart:
            # Send kernel size to ttyUSB0
            uart.write(len(kernel_data).to_bytes(4, byteorder='little'))
            # Write the kernel image to the ttyUSB0 port
            uart.write(kernel_data)
            print(f"kernel_sender: Kernel image sent to {args.device}")
            # Close the ttyUSB0 port
            uart.close()
elif args.type == 'pty':
    print('PTY device:', args.device)
    with open(args.kernel_image, "rb") as kernel:
        kernel_data = kernel.read()
        # Print the size of the kernel image
        print("Kernel size: ", len(kernel_data))
        with open(args.device, "wb", buffering = 0) as tty:
            # Send kernel size to ttyUSB0
            tty.write(len(kernel_data).to_bytes(4, byteorder='little'))
            # Write the kernel image to the ttyUSB0 port
            tty.write(kernel_data)
            print(f"kernel_sender: Kernel image sent to {args.device}")            
            # Close the ttyUSB0 port
            tty.close()
