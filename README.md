# Operating Systems Capstone 2024

| Github Account | Student ID | Name             |
| -------------- | ---------- | ---------------- |
| nella17        | 110550050  | Ting-Shiuan Guan |

## Usage

### Dependency

* `make`
* `clang`
* `llvm`
* `qemu` (optional)
* `minicom` (optional)

### Build

```sh
make bootloader kernel
```

### Pack initramfs

```sh
make fs
```

### Run with QEMU

```sh
# Options:
# TARGET
# DEBUG: build with debug symbol, start qemu gdb server
# QEMU_ASM: dump asm
# QEMU_PTY_SERIAL
make run TARGET=bootloader
make run # default run TARGET=kernel
```

### Upload kernel through UART

```sh
# Options:
# SERIAL: serial device file
make upload
```

### Deploy bootloader to RPI

Currently only support macOS.

```sh
make -C disk format DISK=disk4
make disk
```

## Refs

### RPI OS

* https://github.com/s-matyukevich/raspberry-pi-os
* https://github.com/bztsrc/raspi3-tutorial
* https://github.com/rust-embedded/rust-raspberrypi-OS-tutorials

### Specification

- [BCM2837 ARM Peripherals](https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf)
- [Mailbox property interface](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface)
- [Devicetree Specification v0.4](https://www.devicetree.org/specifications/)
- [Linux and the Devicetree](https://www.kernel.org/doc/html/latest/devicetree/usage-model.html)
- [Device Tree for Dummies](https://bootlin.com/pub/conferences/2013/elce/petazzoni-device-tree-dummies/petazzoni-device-tree-dummies.pdf)
- [ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile](https://developer.arm.com/documentation/ddi0487/aa/?lang=en)
