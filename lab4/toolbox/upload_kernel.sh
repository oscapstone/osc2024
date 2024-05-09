#!/bin/bash

sudo kill $(sudo lsof -t /dev/ttyUSB0)

if [ $? -eq 0 ]; then
    echo "Device is free now. Transmitting the kernel..."
    python3 ./kernel_transmitter.py -d /dev/ttyUSB0
else
    echo "Failed to free the device /dev/ttyUSB0"
fi

sleep 2
sudo screen /dev/ttyUSB0 115200