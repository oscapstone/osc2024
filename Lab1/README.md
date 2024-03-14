# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
|    nien-zhu    | 312515040  |  Nien-Chu Yu  |

## Requirements

* cross-compiler: aarch64-linux-gnu-gcc
* Emulator: qemu-system-aarch64

## Build 

```
make
```

## Debug 

```
make test
make gdb
```

## Test With QEMU

```
make run 
(qemu-system-aarch64 -M raspi3b -serial null -serial stdio -display none -kernel kernel8.img)
```

