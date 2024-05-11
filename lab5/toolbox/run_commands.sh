#!/bin/bash

echo "Sending image to bootloader..."
sudo ./send_image_to_bootloader.py

if [ $? -eq 0 ]; then
    echo "Image sent successfully. Starting screen session..."
    sudo screen /dev/ttyUSB0 115200
else
    echo "Failed to send image to bootloader."
fi
