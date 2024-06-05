#!/bin/sh

sudo apt-get install -y gcc-aarch64-linux-gnu
sudo apt-get install -y qemu-system-aarch64
sudo apt-get install -y gdb-multiarch

sudo pip3 install pyserial
sudo pip3 install pwn
sudo pip3 install argparse
