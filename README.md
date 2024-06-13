# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| jerryyyyy708 | 312551086    | Chi-Jui Chang |

Finalized Head Repo in: https://github.com/jerryyyyy708/osc2024

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
