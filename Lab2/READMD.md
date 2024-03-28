# OSC2024

| Github Account | Student ID |    Name     |
|:--------------:|:----------:|:-----------:|
|   ericlinqq    | 311554046  | Yu-Siou Lin |

## Requirements

* Cross compiler: aarch64-linux-gnu-gcc
* Emulator: qemu-system-aarch64

## Build 

```bash
$ make all ver=release
```

## Debug

```bash
$ make all ver=debug
```

## Build CPIO Archive

```bash
$ mkdir -p rootfs
$ touch rootfs/file.txt
...
$ make rootfs
```

## Test with QEMU

```bash
$ make emu
```

# Interact with Shell and Send Kernel Image

```bash
$ make send --port={port} --image={image}
```
