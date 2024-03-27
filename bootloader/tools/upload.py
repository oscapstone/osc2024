import os
import serial
from pathlib import Path
import argparse


def main(parser: argparse.ArgumentParser):
    args = parser.parse_args()

    uart_path: Path = args.uart_path
    image_path: Path = args.image_path

    s = serial.Serial(port=str(uart_path), baudrate=115200, timeout=1)

    image_size = os.stat(image_path).st_size
    size_bytes = image_size.to_bytes(8, "little")
    s.write(size_bytes)

    received_size = s.readline().decode().strip()
    print(received_size)

    with open(image_path, "rb") as f:
        s.write(f.read())

    received_content = s.readline().decode().strip()
    print(received_content)

    if not args.interactive:
        return

    # go into interactive mode
    s.timeout = 0.5

    # clear buffer
    while (msg := s.readline().decode()) != "":
        print(msg, end="")

    while True:
        cmd = input()
        cmd += "\n"
        s.write(cmd.encode())
        s.flush()

        s.readline()  # clear our message
        while (msg := s.readline().decode()) != "":
            print(msg, end="")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Tranfer kernel image to raspi through uart."
    )
    parser.add_argument("uart_path", type=Path, help="Path to uart serial.")
    parser.add_argument("image_path", type=Path, help="Kernel image path.")
    parser.add_argument("-i", "--interactive", action="store_true")

    main(parser)
