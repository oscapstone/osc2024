import sys

kernel_path = sys.argv[1]
kernel = open(kernel_path, "rb").read()
kernel_size = len(kernel)
print(f"kernel_size: {kernel_size}")

pts = sys.argv[2]
print(f"pts: {pts}")

with open(pts, "wb") as tty:
    # tty.write(b"load\r")
    tty.write(f"{kernel_size}\r".encode())
    tty.write(kernel)
