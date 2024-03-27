# OSC2024

Mini operating system written in Rust.

This is the course project/assignments for "Operating System Capstone," 2024 Spring, at NYCU.

## Table of Contents

- [Student Information](#student-information)
- [Environment Setup](#environment-setup)
- [Dev Notes](#dev-notes)
- [Lab Descriptions](#lab-descriptions)
- [Reference](#reference)

## Student Information

| GitHub Account | Student ID | Name       |
| -------------- | ---------- | ---------- |
| alan910127     | 109652039  | Li-Lun Lin |

## Environment Setup

> [!NOTE]
> Rust uses [LLVM](https://llvm.org/) for its compiler backend, we can get the toolchains needed for cross compiling by simply typing a few commands.

1. Make sure you have [Rust](https://rust-lang.org/) 1.76.0 or above installed

2. Install the target for Raspberry PI 3B+

   ```sh
   rustup target add aarch64-unknown-none-softfloat
   ```

3. Install tools for building the kernel image.

   ```sh
   rustup component add llvm-tools

   # Or `cargo binstall cargo-binutils` if you prefer
   cargo install cargo-binutils
   ```

## Dev Notes

This repository contains two entrypoints that might be great to start from:

- [uartload](crates/uartload)
- [kernel](crates/kernel)

## Lab Descriptions

### Lab 0: Environment Setup ([website](https://nycu-caslab.github.io/OSC2024/labs/lab0.html))

Prepare the environment for developing the mini operating system.

> See [Environment Setup](#environment-setup)

### Lab 1: Hello World ([website](https://nycu-caslab.github.io/OSC2024/labs/lab1.html))

Getting into the world of embedded programming, and try to play with pheripherals.

Tasks:

- [x] **Basic Initialization**: Initialize the memory/registers to be ready to jump into the program.
- [x] **Mini UART**: Setup mini UART to bridge the host and the Raspberry PI.
- [x] **Simple Shell**: Implement a simple shell that display text and read input through mini UART.
- [x] **Mailbox**: Set up the Mailbox service and get the hardware information from it.
- [x] **Reboot**: Add a `reboot` command to the shell to reset the Raspberry PI.

### Lab 2: Booting ([website](https://nycu-caslab.github.io/OSC2024/labs/lab2.html))

Booting the mini operating system, take care of system initialization and preparation

Tasks:

- [x] **UART Bootloader**: Implement a bootloader that loads kernel through mini UART for fast development.
- [ ] **Initial Ramdisk**: Parse "New ASCII Format Cpio" archive file and implement `ls` and `cat`.
- [ ] **Simple Allocator**: Implement a simple allocator that can be used in the early booting stage.
- [x] **Bootloader Self Relocation**: Add self-relocation feature to the bootloader so it does not need to specify the kernel starting address.
- [ ] **DeivceTree**: Integrate DeviceTree support for hardware configuration.

## Reference

- [rust-embedded/rust-raspberrypi-OS-tutorials](https://github.com/rust-embedded/rust-raspberrypi-OS-tutorials)
