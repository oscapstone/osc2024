import serial
import os,sys
import time


# tty.usbserial-0001
# /dev/ttys002
tty = serial.Serial("/dev/ttys003", 115200, timeout=0.5) 
file_stats = os.stat("kernel8.img")

# issue request and tell the size of img to rec
tty.write(str(file_stats.st_size).encode('utf-8'))
# size sended
tty.write("\n")


# send img byte-by-byte
# delay to ensure no loss
with open("kernel8.img", "rb") as fp:
    byte = fp.read(1)
    while byte:
        tty.write(byte)
        byte = fp.read(1)
        #print(byte)
        time.sleep(0.00001)