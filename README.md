# OSC2024

| Github Account | Student ID | Name          |
|----------------|------------|---------------|
| Hst0411 | 312554052    | Shao-Ting Hsu |

## Labs

* a cross-compiler for aarch64
* (optional) qemu-system-arm

### Lab 0: Environment Setup
In Lab 0, you need to prepare the environment for future development. You should install the target toolchain, and use them to build a bootable image for rpi3.
Goals of this lab:

* Set up the development environment.
* Understand the concept of cross-platform development.
* Test your rpi3.

### Lab 1: Hello World
In Lab 1, you will practice bare metal programming by implementing a simple shell. You need to set up mini UART, and let your host and rpi3 can communicate through it.

Goals of this lab:

* Practice bare metal programming.
* Learn how to access rpi3's peripherals.
* Set up the mini UART.
* Set up the mailbox.

### Lab 2: Booting
In Lab 2, you’ll learn one of the methods to load your kernel and user programs. Also, you’ll learn how to match a device to a driver on rpi3. The initialization of the remaining subsystems will be introduced at later labs.

Goals of this lab:

* Implement a bootloader that loads kernel images through UART
* Implement a simple allocator.
* Understand what’s initial ramdisk.
* Understand what’s devicetree.
