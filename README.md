# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| RobertttBS     | 311555026  | 謝柏陞         |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm
* Modification at Makefile

## Build kernel img

```
make
```

## Build user program and produce "initramfs.cpio"
```
./compile_user_program.sh
```

## Test kernel8.img with QEMU

```
make run
```

## Test tty with QEMU
* `make sendimg` and `make commu` is not applicable to this, since the port is not the same.
```
make tty
```

## Send image to Raspi 3

```
make sendimg
```

## Communicate with Raspi 3 after `make sendimg`

```
make commu
```
