import serial
import os
import os.path
import sys
import time

BAUD_RATE = 115200

def read_line(s):
    received_string = ""
    while True:
        try:
            c = s.read(1).decode()
        except UnicodeDecodeError:
            c = s.read(1).decode()

        if c=="\r":
            continue
        if c=="\n":
            break
        received_string += c
    return received_string

def send_img(ser, kernel):
    print(read_line(ser))
    
    kernel_size = os.stat(kernel).st_size
    print((str(kernel_size) + "\n").encode())
    ser.write(("."+ str(kernel_size) + "\n").encode())

    print("kernel size is ", read_line(ser))
    # print(read_line(ser))
    print(read_line(ser))

    # 將映像檔逐一傳輸到對方端
    with open(kernel, "rb") as image:
        while kernel_size > 0:
            ser.write(image.read(1))
            kernel_size -= 1
            # ser.read_until(b".")
    # 讀取對方端傳來的結束訊息
    print(read_line(ser))
    print(read_line(ser))
    print(ser.read_until("> ").decode(), end="")
    return

# 程式進入點
if __name__ == "__main__":
    # 設定串列通訊的埠號和傳輸速率，以及逾時時間
    # ser = serial.Serial("/dev/ttyUSB0", BAUD_RATE, timeout=5)
    ser = serial.Serial("COM10", BAUD_RATE, timeout=5)

    # 傳輸映像檔
    send_img(ser, "kernel8.img")