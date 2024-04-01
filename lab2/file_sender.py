import serial
import time
import sys

kernel_image_path = "kernel/kernel8.img"

dev_name = sys.argv[1]
ser = serial.Serial(dev_name, 115200, timeout=1)

input("Press any button to continue...")

with open(kernel_image_path, "rb") as f:
    kernel_image = f.read()

kernel_size = len(kernel_image)
print(f"Sending kernel size: {kernel_size} bytes")
ser.write(f"{kernel_size}\n".encode())

time.sleep(1)

print("Sending kernel image...")
ser.write(kernel_image)

ser.close()
print("Kernel image sent.")
