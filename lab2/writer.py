import serial
import os
import time


tty = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.5) 
# acquire the file size
file_stats = os.stat("./shell/shell.img")
# issue request and tell the size of img to rec
tty.write(str(file_stats.st_size).encode('utf-8'))
# size sended
# python3 .encode()
tty.write(str("\n").encode('utf-8'))
time.sleep(0.0005)
# send img byte-by-byte
# delay to ensure no loss
# uart is low speed interface
# if sleep too short e.g: 0.0001, it may loss
with open("./shell/shell.img", "rb") as fp:
    byte = fp.read(1)
    while byte:
        tty.write(byte)
        byte = fp.read(1)
        # delay enough time to ensure no loss
        time.sleep(0.0005)