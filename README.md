# OSC2023

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| ZoLArk173 | 109550083    | Hao-Yu Yang |

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