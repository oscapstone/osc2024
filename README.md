# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| psychicalcoder | 311510189    | 郭迺平 |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

## Build 

```
make 
```

## Test With QEMU

```
qemu-system-aarch64 -M raspi3b -kernel kernel8.img -display none -serial null -serial stdio
```
