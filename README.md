# OSC2024

| GitHub Account | Student ID | Name        |
| -------------- | ---------- | ----------- |
| ppodds         | B111505    | Hung-Yi Liu |

## 實驗環境

- Ubuntu 22.04
- x86-64
- Rust nightly-2024-05-23

## Rust Install

```shell
# C toolchain
sudo apt install build-essential crossbuild-essential-arm64
# Install rustup and default toolchain
curl https://sh.rustup.rs -sSf | sh
# Reload shell environment
source "$HOME/.cargo/env"
```

## QEMU Install

```shell
sudo apt install qemu-system-arm
```

## GDB Install

```shell
# Install multi arch gdb
sudo apt install gdb-multiarch
# Install GEF (gdb plugin)
bash -c "$(wget https://gef.blah.cat/sh -O -)"
```

## Build Executable And Binary Image

```shell
make
```

## Run In QEMU

```shell
make qemu
```

## Debug In QEMU

```shell
make qemu-debug
```
