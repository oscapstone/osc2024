#!/bin/bash

if [ $# -eq 0 ]; then
    device="/dev/ttyUSB0"
else
    index="$1"
    device="/dev/pts/$index"
fi
    
if [ ! -c "$device" ]; then
    echo "Device not found: $device"
    exit 1
fi

echo "screen device : $device"
sudo screen "$device" 115200

