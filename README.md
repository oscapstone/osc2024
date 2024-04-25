# OSC2023

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| flyotlin | 312551020    | Po-Ru Lin |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

## Build 

```
make kernel.img
```

## Test With rpi3b+

- cpio
  - copy to SD card
  - config.txt
    - `initramfs initramfs.cpio 0x20000000`
- dtb
  - 

## Test With QEMU

- cpio
  - `-initrd <cpio archive>`
  - 0x8000000
- Run bootloader
  - `make clean & make`
  - `make run-pty`
  - `python3 uart.py`
    - wait 5 seconds
  - `screen /dev/pts/xxx`

```
qemu-system-aarch64 -M raspi3b -kernel kernel.img -initrd initramfs.cpio -serial null -serial stdio -dtb bcm2710-rpi-3-b-plus.dtb
```