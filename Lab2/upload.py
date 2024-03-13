import serial
import os

file_name = "kernel8.img"
file_stats = os.stat(file_name)
file_stats_byte = file_stats.st_size;
print(file_stats)
# st_size property to get the file size in bytes.
print(f'File Size in Bytes is {file_stats_byte}')
#print(f'{file_stats_byte.to_bytes(4,byteorder="little")}')


with open('/dev/pts/4', "wb", buffering = 0) as tty:
  tty.write(file_stats_byte.to_bytes(4,byteorder="little"))
  
  with open(file_name, "rb") as kernel_img:
    tty.write(kernel_img.read())