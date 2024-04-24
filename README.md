# OSC2023

| Github Account | Student ID | Name        |
|----------------|------------|-------------|
| ZoLArk173      | 109550083  | Hao-Yu Yang |

## Requirements

* a cross-compiler for aarch64
* (optional) qemu-system-arm

```
rustup target add aarch64-unknown-none-softfloat
```

## Build 

```
make build
```

## Test With QEMU

```
make run
```

## Misc.

### Used memory address
* `0x7_5000`: Critical Section counter
* `0x8_F000`: Device Tree temporary address
* Start from `0x8_0000`: Code section
* Start from `0x6_0000`: Boot code section
* End from `0x7_0000`: Stack
* `0x910_0000` to `0x2000_0000`: Heap
