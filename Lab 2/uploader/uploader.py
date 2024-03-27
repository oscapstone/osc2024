#! /usr/bin/python3

import os
from socket import timeout
import sys
import serial
from time import sleep


def send_image(ser):
    kernel = '../kernel/out/kernel8.img'
    kernel_size = os.stat(kernel).st_size
    print(f"sending kernel size: {kernel_size} ... ", end='')
    ser.write((str(kernel_size) + "\n").encode())
    sleep(0.5)
    ser.read_until(b"$loading$")
    print('ok.')
    print('sending kernel ... ', end='')
    with open(kernel, "rb") as f:
        ser.write(f.read())
    ser.read_until(b"$done$") 
    print('done.')


def serial_shell(ser):
    try:
        while True:
            feedback = ""
            while ser.in_waiting:
                feedback = ser.read(ser.in_waiting).decode()
                print(feedback)
            if "$size$" in feedback:
                send_image(ser)
            else:
                key_in = (input("#> ").lower() + "\n")
                if 'exit' in key_in:
                    ser.close()
                    print('\nbye!')
                    sys.exit()
                ser.write(key_in.encode())
                sleep(0.1)

    except KeyboardInterrupt:
        ser.close()
        print('\nbye!')
    

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/tty.usbserial-0001"
    try:
        ser = serial.Serial(port, 115200, timeout=50)
        serial_shell(ser)
    except serial.serialutil.SerialException as e:
        print(e);    
    


if __name__ == "__main__":
    main()
