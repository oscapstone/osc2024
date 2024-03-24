import argparse
import serial
import os
from time import sleep

def parse_arguments():
    parser = argparse.ArgumentParser(description='Send an image to a server')
    parser.add_argument('--device', type=str, default='/dev/ttyUSB0', help='Path to the device')
    parser.add_argument('--image', type=str, default='/home/eric/Desktop/osc2024/Lab2/kernel/kernel8.img', help='Path to the image')
    parser.add_argument('--baudrate', type=int, default=115200, help='Baudrate of the device')
    return parser.parse_args()

def send_img(device, image, baudrate):
    img_size = os.stat(image).st_size
    ser = serial.Serial(device, baudrate, timeout=5)

    print(ser.read_until(b"Please send the image size:\r\n").decode())
    ser.write(img_size.to_bytes(4, byteorder='little'))

    sleep(0.5)

    print(ser.read_until(b"Start to load the kernel image...\r\n").decode())

    with open(image, 'rb') as f:
        ser.write(f.read())

    print(ser.read_until(b"kernel loaded\r\n").decode())

if __name__ == '__main__':
    args = parse_arguments()
    send_img(args.device, args.image, args.baudrate)
