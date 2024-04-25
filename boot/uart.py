import argparse
import os
import subprocess
import time


class Kernel:
    def __init__(self):
        self.pwd = os.path.abspath(os.path.dirname(__file__))
        self.kernel_dir = os.path.join(self.pwd, "../kernel")
        self.kernel_img = os.path.join(self.pwd, "../kernel/dist/kernel8.img")

    def build(self):
        # TODO: error handling
        subprocess.run(["make", "clean"], cwd=self.kernel_dir)
        subprocess.run(["make"], cwd=self.kernel_dir)

    def stat(self) -> os.stat_result:
        return os.stat(self.kernel_img)

    def upload(self, device: str):
        stat = self.stat()
        print("start upload...")
        with open(device, "wb", buffering=0) as tty:
            print("Wait 5 seconds")
            time.sleep(5)
            print("Upload header...")

            # hdr = f"MC{stat.st_size}CMe".encode()
            # for i in range(len(hdr)):
            #     print(i+1, hdr[i: i+1])
            #     tty.write(hdr[i: i+1])
            #     time.sleep(0.01)
            tty.write(f"MC{stat.st_size}CMe".encode())   # qemu: prepend "M"
            # TODO: check written number correct

            print("Header written")
            time.sleep(1)
            print("Upload kernel...")
            total = 0

            with open(self.kernel_img, "rb") as kernel:
                # count = 0
                while True:
                    data = kernel.read(1)
                    # print(f'get: {len(data)}')
                    total += len(data)
                    if not data:
                        print("no data available")
                        break
                    tty.write(data)
                    # count += 1
                    # if count == 1024:
                    #     time.sleep(0.1)
                    #     count = 0
                    # print(f'progress: {total}/{stat.st_size}')
                    # time.sleep(0.001)
            print('total: ', total)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-build", action='store_true', help="Skip kernel build")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--rpi3", action='store_true', help="Run on rpi3b+")
    group.add_argument("--qemu", help="Run on qemu")

    args = parser.parse_args()
    print(args)

    kernel = Kernel()
    if not args.no_build:
        kernel.build()

    if args.rpi3:
        kernel.upload(device="/dev/ttyUSB0")
    if args.qemu is not None:
        kernel.upload(device=args.qemu)


if __name__ == "__main__":
    main()