# OSC 2024

| Github Account | Student ID | Name         |
|----------------|------------|--------------|
| Ruccccc        | 110652011  | Chia-Han Kuo |

## Build

```
make kernel.img
```

## Test With QEMU

```
qemu-system-aarch64 -M raspi3b -kernel kernel.img -initrd initramfs.cpio -serial null -serial stdio -dtb bcm2710-rpi-3-b-plus.dtb
```
