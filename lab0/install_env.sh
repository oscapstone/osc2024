#!/bin/sh

sudo apt-get install -y gcc-aarch64-linux-gnu
sudo apt-get install -y qemu-system-aarch64
sudo apt-get install -y gdb-multiarch

pip3 install pyserial
pip3 install pwn
pip3 install argparse
