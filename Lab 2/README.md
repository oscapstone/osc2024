# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| KentLee0820    | 312551131  | Chien-Wei Lee |


## Requirements

* cross-compiler : aarch64-linux-gnu-gcc
* emulator : qemu-system-aarch64

## Build cpio file

In lab2 directory:

```
make all
```

## Build bootloader

In the bootloader subdirectory

```
build : make all
qemu to stdio : make run
qemu to serial : make pty
```

Note: command of testing Bootloader command in QEMU

```
qemu-system-aarch64 -M raspi3b -kernel bootloader.img -display none -serial null -serial pty
     	-initrd initrd.cpio -dtb bcm2710-rpi-3-b-plus.dtb
```
 
# Build kernel

In the kernel subdirectory

```
build : make all
qemu to stdio : make run
```
