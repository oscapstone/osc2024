# 指定使用 python3 執行此檔案
#! /usr/bin/python3

# 導入必要的模組
import os
from socket import timeout
import time
import sys
import serial
from time import sleep

# 設定串列通訊的傳輸速率
BAUD_RATE = 115200

# 定義函式，用於傳輸映像檔
def send_img(ser, kernel):
    # 請求輸入映像檔大小
    print("Please sent the kernel image size:")
    sleep(5)
    kernel_size = os.stat(kernel).st_size
    # 將映像檔大小傳輸到對方端
    ser.write((str(kernel_size) + "\n").encode())
    # 讀取對方端的回應
    print(ser.read_until(b"Start to load the kernel image... \r\n").decode(), end="")

    # 將映像檔逐一傳輸到對方端
    with open(kernel, "rb") as image:
        while kernel_size > 0:
            kernel_size -= ser.write(image.read(1))
            # ser.read_until(b".")
    # 讀取對方端傳來的結束訊息
    print(ser.read_until(b"> ").decode(), end="")
    return

# 程式進入點
if __name__ == "__main__":
    # 設定串列通訊的埠號和傳輸速率，以及逾時時間
    # ser = serial.Serial("/dev/ttyUSB0", BAUD_RATE, timeout=5)
    ser = serial.Serial("COM10", BAUD_RATE, timeout=5)

    # 傳輸映像檔
    send_img(ser, "kernel8.img")