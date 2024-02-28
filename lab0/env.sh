#!/bin/bash

sudo apt update
sudo apt install build-essential git -y

# install the cross-compiler
sudo apt install gcc-aarch64-linux-gnu -y

# install the qemu emulator for arm
sudo apt install qemu-system-arm -y

# install screen for connecting to rpi
sudo apt install screen
