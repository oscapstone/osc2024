import os
import serial
import argparse
import time

'''
# move bootload.img and config.txt to sd card
cd things_to_sd_card/rootfs                                         
find . ! -name '*.DS_Store' | cpio -o -H newc > ../initramfs.cpio
cd ../..
# minicom -D /dev/tty.usbserial-0001
# python3.9 send_img_uart.py -t /dev/tty.usbserial-0001 -i ./kernel8.img
# ESC-x

gdb
file ./build/kernel8.elf
target remote :1234
'''

BAUD_RATE = 115200

def read_line(s):
    received_string = ""
    while True:
        c = s.read().decode()
        if c == "\r":
            continue
        if c == "\n":
            break
        received_string += c
    return received_string

def send_img(tty, image):
    ser = serial.Serial(tty, BAUD_RATE)

    img_size = os.stat(image).st_size
    print(f"S: Kernel image size: {img_size}")
    ser.write((str(img_size)+"\n").encode())
    # print("R: "+read_line(ser))

    print("S: Sending kernel image...")
    cnt = 0
    with open(image, "rb") as img:
        byte = img.read(1)
        while byte:
            ser.write(byte)
            img_size -= 1
            # cnt += 1
            # if cnt % 100 == 0:
            #     print("R: "+read_line(ser))
            byte = img.read(1)
            time.sleep(0.0005)

    # print("R: "+read_line(ser))
    # print("R: "+read_line(ser))

if __name__ =="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--tty", help="TTY device that want to receive", required=True)
    parser.add_argument("-i", "--image", help="image path that want to send", required=True)
    args = parser.parse_args()

    send_img(args.tty, args.image)
    