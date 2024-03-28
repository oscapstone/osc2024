#!/bin/bash
source ../../bin/activate
sudo chmod a+rw /dev/ttyUSB0
python3 uploader.py