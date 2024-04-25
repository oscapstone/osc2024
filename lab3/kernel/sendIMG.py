import serial
import os,sys
import time


# tty.usbserial-0001
# /dev/ttys002
# designate port and the baut rate
tty = serial.Serial("/dev/tty.usbserial-0001", 115200, timeout=0.5) 
file_stats = os.stat("kernel8.img")

# issue request and tell the size of img to rec
tty.write(str(file_stats.st_size).encode('utf-8'))
# size sended
# python3 .encode()
tty.write("\n")
time.sleep(0.0001)

# send img byte-by-byte
# delay to ensure no loss
# uart is low speed interface
# if sleep too short e.g: 0.0001, it may loss
with open("kernel8.img", "rb") as fp:
    byte = fp.read(1)
    while byte:
        tty.write(byte)
        byte = fp.read(1)
        # delay enough time to ensure no loss
        time.sleep(0.0001)