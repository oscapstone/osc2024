import serial
import os
import time

# open serial port
tty = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.5)
file_stats = os.stat("img/kernel8.img")

# send file size
endline = "\n"
tty.write(str(file_stats.st_size).encode("utf-8"))
tty.write(endline.encode("utf-8"))

# send file content byte by byte
with open("img/kernel8.img", "rb") as fp:
    byte = fp.read(1)
    while byte:
        tty.write(byte)
        byte = fp.read(1)
        time.sleep(0.0001)  # delay to avoid buffer overflow
