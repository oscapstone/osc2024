import argparse
import serial
import os
from tqdm import tqdm, trange
import math
import time



def main():
    # parsing argument
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file", help="file to be transfered", type=str)
    parser.add_argument("-p", "--port", help="transfer portination", type=str)
    args = parser.parse_args()
    if args.file is None or args.port is None:
        print("Usage: python3 -f <file> -d <port>")

    # read kernel file
    with open(args.file, "rb") as file:
        bytecodes = file.read()
    # Describe kernel size
    size = os.path.getsize(args.file)
    print(f"File {args.file}: {size} bytes")

    # Open serial port to device
    try:
        port = serial.Serial(args.port, 115200)
    except:
        print("Serial Error!")
        exit(1)
    
    port.write(b'help\n')
    text = port.read_until(b'$ ').decode(encoding='ascii')

    for i in range(0, len(bytecodes), 4):
        while True:
            addr = hex(int("0x80000", 16)+i)
            value = bytecodes[i:i+4][::-1].hex() 
            print(f"{addr} {value: <8} {i: >6}/{len(bytecodes)}", end="")
            port.write(f'write {addr} {value}\n'.encode('ascii'))
            port.flush()
            text = port.read_until(b'$ ').decode(encoding='ascii')
            text = text.split("\n")[0].split(" ")
            ret_addr = text[1]
            ret_value = text[2]
            left_icons = round(10*i/len(bytecodes))
            right_icons = 10 - left_icons
            print(f" progress: {'🌌'*left_icons}{'🌠' if right_icons > 0 else '🌟'}{'⬛'*right_icons}", end="\n")
            print('\033[F', end="")
            if ret_addr == addr and ret_value == value:
                break
    print("\nfinish!")
    port.write(f'jump 0x80000\n'.encode('ascii'))
    port.flush()
        

    # # Start trasfering kernel
    # while not port.writable():
    #     pass
    # port.write(size.to_bytes(4, byteorder='big'))
    # port.flush()
    
    # chunk_size = 1
    # chunk_cnts = math.ceil(size/chunk_size)
    # for i in tqdm(range(chunk_cnts), f"Transfering bytes"):
    # # for i in range(chunk_cnts):
    #     while not port.writable():
    #         pass
    #     port.write(bytecodes[i*chunk_size: (i+1)*chunk_size])
    #     port.flush()
    #     time.sleep(0.001)
        

if __name__ == "__main__":
    main()
