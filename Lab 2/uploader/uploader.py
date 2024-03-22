#! /usr/bin/python3

import os
from socket import timeout
import time
import sys
import serial
from time import sleep


BAUD_RATE = 115200


def send_img(ser,kernel):
    pass



if __name__ == "__main__":
    ser = serial.Serial("/dev/ttyUSB0", BAUD_RATE, timeout=5)
    # send_img(ser,"../kernel8.img")