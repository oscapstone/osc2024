# OSC2024

| Github Account | Student ID | Name          |
| -------------- | ---------- | ------------- |
| AxelHowe       | 312552019  | 蘇家弘        |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

## Build 

```
make run
```

## Test With QEMU

```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio
```