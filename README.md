# OSC2024

| Github Account | Student ID |    Name     |
|:--------------:|:----------:|:-----------:|
|   ericlinqq    | 311554046  | Yu-Siou Lin |

## Requirements

* Cross compiler: aarch64-linux-gnu-gcc
* Emulator: qemu-system-aarch64

## Build 

```
make ver=release
```

## Debug (emulation + debug)

```
make ver=debug
```

## Test With QEMU

```
make emu
(qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none)
```
