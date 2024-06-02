#!/bin/sh

sudo apt-get install -y gcc-aarch64-linux-gnu
sudo apt-get install -y qemu-system-aarch64
sudo apt-get install -y gdb-multiarch
sudo apt-get install -y python3-pip

python3 -m pip install pyserial
python3 -m pip install pwn
python3 -m pip install argparse
