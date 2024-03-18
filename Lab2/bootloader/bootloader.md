# OSC2024 Lab2
### Bootloader
Load without relocate:
1. Place ``config.txt`` and ``bootloader.img`` into SD card.
2. Connect to raspi3b+ with command
```
sudo screen /dev/ttyUSB0 115200
```
3. Type in "boot" in serial port to ensure the loader is waiting for file.
4. Open another terminal in the lab2 folder and use ``send_loader.py`` : 
```
python3 send_loader.py -s [serial path] -f [kernel file]
//default -s /dev/ttyUSB0 -f kernel8.img
```
### TODOs
* ``bootloader_main.c``: file receive logic
* ``send_loader.py``: try open and write, read_line logic
* relocate