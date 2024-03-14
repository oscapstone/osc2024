# OSC2023

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| Haoze0102      | A122542    | Hao-Ze Wang   |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

## Build 

```
make kernel.img
```

## Test With QEMU

```
qemu-system-aarch64 -M raspi3b -kernel kernel.img -initrd initramfs.cpio -serial null -serial stdio -dtb bcm2710-rpi-3-b-plus.dtb
```