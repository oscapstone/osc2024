import serial
import argparse

parser = argparse.ArgumentParser(description='OSC 2024')
parser.add_argument('--filename', metavar='PATH',
                    default='kernel8.img', type=str, help='path to script')
parser.add_argument('--device', metavar='TTY',
                    default='/dev/ttyUSB0', type=str,  help='path to UART device')
parser.add_argument('--baud', metavar='Hz', default=115200,
                    type=int,  help='baud rate')

args = parser.parse_args()

with open(args.filename, 'rb') as file: #binary read
    with serial.Serial(args.device, args.baud) as ser:
        raw = file.read()
        length = len(raw)
        length_sent = len(raw)

        print(f'image size: {length}')

        while length_sent > 0:
            digit = length_sent % 10
            length_sent = length_sent // 10

            ser.write(str(digit).encode()) #default is UTF-8
            ser.flush()

        ser.write(str('\n').encode()) #size sending is done
        ser.flush()
        
        print("start sending...") #send raw data of img to UART
        for i in range(length):
            ser.write(raw[i:i+1])
            ser.flush()
            
        print("sended successfully")
        
