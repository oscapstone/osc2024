# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| kaoyuhung      | 109550040  | Yu-Hung Kao   |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

## Build 

```
make
```

## Test With QEMU
### Check dumped assembly

```
make asm
```

### Use QEMU to run
```
make run
```

### Use QEMU to debug
```
make debug
```

### Create a pseudo TTY device for QEMU to test the bootloader
```
make tty
```