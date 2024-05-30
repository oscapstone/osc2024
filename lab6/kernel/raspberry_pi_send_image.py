import os
import serial

def main():
    COM_PORT = "COM3"
    BAUD_RATES = 115200
    ser = serial.Serial(COM_PORT, BAUD_RATES)

    size = os.stat("./kernel8.img").st_size
    print(size)
    ser.write((size).to_bytes(4, byteorder='little'))

    for i in (range(4)):
        c = ser.read()
        print(c)

    with open("./kernel8.img", "rb", buffering = 0) as kernel:
        ser.write(kernel.read())

    resp = ""
    while True:
        c = ser.read().decode()
        if(c == '\n'):
            break
        resp += c
    print(resp)

    ser.close()

if __name__ == "__main__":
    main()