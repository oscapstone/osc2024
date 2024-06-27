# import serial
import os
import time

kernel_size = os.path.getsize('kernel8.img')

size_bytes = kernel_size.to_bytes(4, byteorder='big')
print(size_bytes)
with open('/dev/ttyUSB0', 'wb', buffering=0) as tty:
    tty.write(size_bytes)


time.sleep(0.1)  # 添加一些延迟以确保数据完全发送

with open('kernel8.img', 'rb') as kernel_file:
    kernel_data = kernel_file.read()

print("start transfer\n")
# /dev/ttyUSB0
with open('/dev/ttyUSB0', 'wb', buffering=0) as tty:
    tty.write(kernel_data)
    # tty.flush()  # 确保所有数据都被发送

print("finish\n")