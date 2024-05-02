import serial
import os
import time
from tqdm import tqdm

def send_file(serial_port, file_path, baud_rate=115200, timeout=0.5, sleep_time=0.002):

    tty = serial.Serial(serial_port, baud_rate, timeout=timeout)
    
    file_size = os.path.getsize(file_path)

    tty.write(str(file_size).encode('utf-8'))
    tty.write("\n".encode('utf-8'))
    time.sleep(sleep_time)

    pbar = tqdm(total=file_size, unit="B", unit_scale=True, desc="Sending")

    start_time = time.time()
    
    with open(file_path, "rb") as fp:
        for byte in iter(lambda: fp.read(1), b''):
            tty.write(byte)
            pbar.update(len(byte))
            time.sleep(sleep_time)

    pbar.close()

    total_time = time.time() - start_time

    print(f"Transfer completed in {total_time:.2f} seconds. Speed: {file_size / total_time:.2f} Bytes/s")

send_file("/dev/ttyUSB0", "kernel8.img")
