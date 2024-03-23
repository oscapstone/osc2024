## Process
First of all, some files should be put into the SD card:
```
bcm2710-rpi-3-b-plus.dtb
config.txt
initramfs.cpio
bootloader.img
```
- `bcm2710-rpi-3-b-plus.dtb`: device tree
- `config.txt`: rpi3 configuration file
- `initramfs.cpio`: cpio archive 
- `bootloader.img`: bootloader which will receive `kernel.img` via UART then execute the kernel

Send `kernel.img` via UART:
```
python3 send_kernel.py "/dev/cu.usbserial-0001"
```

Connect to the shell:
```
screen /dev/cu.usbserial-0001 115200