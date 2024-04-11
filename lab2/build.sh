#!/bin/bash

rm ./build/*
cd kernel
make
cd ..
rm ./build/start.o
cd bootloader
make
