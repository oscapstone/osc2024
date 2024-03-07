import serial
import os
import time

path = '/dev/ttyUSB0'
file_name = "kernel8.img"
file_stats = os.stat(file_name)
file_stats_byte = file_stats.st_size;
print(file_stats)
# st_size property to get the file size in bytes.
print(f'File Size in Bytes is {file_stats_byte}')
#print(f'{file_stats_byte.to_bytes(4,byteorder="little")}')

with open(path, "w") as tty:
	tty.write('!')
time.sleep(3)
with open(path, "wb", buffering = 0) as tty:
  tty.baudrate = 115200
  tty.write(file_stats_byte.to_bytes(4,byteorder="little"))
  
  with open(file_name, "rb") as kernel_img:
    #if file_stats_byte < 4090:
     #  tty.write(kernel_img.read())
    #else:
    bytes_written = 0
    while True:
        chunk = kernel_img.read(1000)  # Read 1000 bytes at a time
        if not chunk:
            break  # End of file
        tty.write(chunk)  # Write the chunk to the tty
        bytes_written += len(chunk)  # Update the total bytes written
        if bytes_written >= 1000:
            time.sleep(1)  # Wait for 1 second after writing 1000 bytes
            bytes_written = 0  # Reset the byte count
