# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| KentLee0820    | 312551131  | Chien-Wei Lee |


## Requirements

* cross-compiler : aarch64-linux-gnu-gcc
* emulator : qemu-system-aarch64


## Build 

```
release : make all
debug : make debug
```

## Test With QEMU

```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio
```
 