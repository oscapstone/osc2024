#!/usr/bin/env python3

from serial import Serial
from pwn import *
import argparse
from sys import platform

if platform == "linux" or platform == "linux2":
    parser = argparse.ArgumentParser(description='NYCU OSDI kernel sender')
    parser.add_argument('--filename', metavar='PATH', default='kernel8.img', type=str, help='path to kernel8.img')
    parser.add_argument('--device', metavar='TTY',default='/dev/ttyUSB0', type=str,  help='path to UART device')
    parser.add_argument('--baud', metavar='Hz',default=115200, type=int,  help='baud rate')
    args = parser.parse_args()

    with open(args.filename,'rb') as fd:
        with Serial(args.device, args.baud) as ser:

            kernel_raw = fd.read()
            length = len(kernel_raw)

            print("Kernel image size : ", hex(length))
            for i in range(8):
                ser.write(p64(length)[i:i+1])
                ser.flush()

            print("Start sending kernel image by uart1...")
            for i in range(length):
                # Use kernel_raw[i: i+1] is byte type. Instead of using kernel_raw[i] it will retrieve int type then cause error
                ser.write(kernel_raw[i: i+1])
                ser.flush()
                if i % 100 == 0:
                    print("{:>6}/{:>6} bytes".format(i, length))
            print("{:>6}/{:>6} bytes".format(length, length))
            print("Transfer finished!")

else:
    # 引入 argparse 庫用於解析命令行參數
    parser = argparse.ArgumentParser(description='NYCU OSDI kernel sender')
    # 添加 --filename 參數，用於指定要發送的內核映像文件的路徑
    parser.add_argument('--filename', metavar='PATH', default='kernel8.img', type=str, help='path to kernel8.img')
    # 添加 --device 參數，用於指定 UART 設備的設備文件路徑
    parser.add_argument('--device', metavar='COM', default='COM3', type=str, help='COM# to UART device')
    # 添加 --baud 參數，用於指定 UART 通信的波特率
    parser.add_argument('--baud', metavar='Hz', default=115200, type=int, help='baud rate')
    # 解析命令行參數
    args = parser.parse_args()

    # 以二進制讀取模式打開內核映像文件
    with open(args.filename,'rb') as fd:
        # 打開 UART 設備
        with Serial(args.device, args.baud) as ser:

            # 讀取文件內容到 kernel_raw
            kernel_raw = fd.read()
            # 獲取文件長度
            length = len(kernel_raw)

            # 打印內核映像的大小
            print("Kernel image size : ", hex(length))
            # 先發送文件長度資訊，這裡假定使用了 64 位的長度編碼
            for i in range(8):
                # 將 length 以 64 位小端序格式進行打包，然後逐字節發送
                ser.write(p64(length)[i:i+1])
                ser.flush()

            # 開始通過 UART 發送內核映像數據
            print("Start sending kernel image by uart1...")
            for i in range(length):
                # 發送每個字節，並在每發送 100 個字節後打印進度資訊
                ser.write(kernel_raw[i: i+1])
                ser.flush()
                if i % 100 == 0:
                    print("{:>6}/{:>6} bytes".format(i, length))
            # 最後打印發送完成的資訊
            print("{:>6}/{:>6} bytes".format(length, length))
            print("Transfer finished!")
