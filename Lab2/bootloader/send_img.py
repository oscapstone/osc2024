import argparse
import serial
import os
from time import sleep
import sys

def parse_arguments():
    parser = argparse.ArgumentParser(description='Send an image to a server')
    parser.add_argument('--device', type=str, default='/dev/ttyUSB0', help='Path to the device')
    parser.add_argument('--image', type=str, default='/home/eric/Desktop/osc2024/Lab2/kernel/kernel8.img', help='Path to the image')
    parser.add_argument('--baudrate', type=int, default=115200, help='Baudrate of the device')
    return parser.parse_args()

def send_img(serial, image):
    img_size = os.stat(image).st_size

    serial.write(img_size.to_bytes(4, byteorder='little'))

    sleep(0.5)

    print(serial.read_until(b"Start to load the kernel image...\r\n").decode("ascii"))

    with open(image, 'rb') as f:
        serial.write(f.read())

    print(serial.read_until(b"Done\r\n").decode("ascii"));
    print(serial.read(serial.in_waiting).decode("ascii"))

def shell_interact(device, image, baudrate):
    try:
        ser = serial.Serial(device, baudrate, timeout=5)
    except serial.serialutil.SerialException as e:
        print(e)
        sys.exit()

    try:
        while True:
            feedback = ""
            while ser.in_waiting:
                try:
                    feedback = ser.read(ser.in_waiting).decode("ascii")
                    print(feedback, end="")
                except UnicodeDecodeError:
                    break
            if "Please" in feedback:
                send_img(ser, image)
                # ser.close()
                # sys.exit()
            else:
                key_in = (input() + "\n").encode("ascii")
                ser.write(key_in)
                sleep(0.1)
    except KeyboardInterrupt:
        ser.close()
        sys.exit()

if __name__ == '__main__':
    args = parse_arguments()
    shell_interact(args.device, args.image, args.baudrate)
