#!/bin/bash

PID=$(sudo lsof /dev/ttyUSB0 | grep 'screen' | awk '{print $2}')
if [ -n "$PID" ]; then
    echo "Killing process using /dev/ttyUSB0: $PID"
    sudo kill $PID
fi

# send img
echo "Sending image file..."
python3 ./send_img.py & #讓python code在背景執行

# 等python code執行2.5秒後開啟screen 這個值需要根據kernel大小決定，或是把前面的&拿掉乖乖等python傳完再開screen
sleep 2.5
echo "Reopening screen session on /dev/ttyUSB0 at 115200 baud rate..."
sudo screen /dev/ttyUSB0 115200

echo "Script completed."