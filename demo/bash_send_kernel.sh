#!/bin/bash

## rapspi -> bash send_kernel.sh
## qemu   -> bash send_kernel.sh i

## Construct the device path
if [ $# -eq 0 ]; then
    device="/dev/ttyUSB0"
else
    index="$1"
    device="/dev/pts/$index"
fi

# Check if the device file exists
if [ ! -c "$device" ]; then
    echo "Device not found: $device"
    exit 1
fi

# Change permissions of the device file
echo chmod a+rw "$device"
sudo chmod a+rw "$device"

# Execute send_kernel.py with the appropriate argument
python3 send_kernel.py $1
