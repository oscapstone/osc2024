#!/bin/bash

# This script sends the kernel image to Rpi3 through UART

DEST_PATH="/dev/ttyUSB0"
KERNEL_PATH="../kernel8.img"

# Check the root permission
if [ "$(id -u)" != "0" ]; then
    echo "This script must be run as root"
    exit 1
fi

# Get the size of the kernel image file and send it to Rpi3
# wc -c: count bytes of a file
# tr -d: delete characters
# sleep: wait n seconds
wc -c < $KERNEL_PATH | tr -d '\n' > $DEST_PATH | sleep 1

# Send the kernel image
# pv: redirect file input to specified tty
pv $KERNEL_PATH --rate-limit 100 > $DEST_PATH