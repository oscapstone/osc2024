# Lab1 Implement a simple shell on Rpi3

- [raspi-os-L1](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/rpi-os.md)


Explainations is given in comments of source code.

Here is suggested code review step
- Makefile
- boot.S
- kernel.c
- mini_uart.c / mini_uart.h
- shell.h / shell.c


## UART
UART stands for universal asynchronous receiver-transmitter. UART is a protocol for configural data format and transmission speeds asynchronous communication.

Baudrate
- 115200 baudrate means transmit 115200 bits every second

### BCM2837 ARM Peripherals manual
We connect raspi3 with UART. UART is a peripheral of raspi3, so we have to read manual if we want to let host computer communicate with raspi3.

Raspi3 reserves the memory above `0x3F000000` for device (peripheral memory post-mapping), 

Reference
- [BCM2837 ARM Peripherals manual](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)
- [UART](https://nycu-caslab.github.io/OSC2024/labs/hardware/uart.html#uart)

In `BCM2837 Manual` 1.2.3 ARM physical addresses
> Physical addresses range from `0x3F000000` to `0x3FFFFFFF` for peripherals. The bus addresses for peripherals are set up to map onto the peripheral bus address range starting at `0x7E000000`. Thus a peripheral advertised here at bus address `0x7Ennnnnn` is available at physical address `0x3Fnnnnnn`.

Our code run directly on CPU, so we should use physical address `0x3Fnnnnnn`.

Besides, `BCM2837 Manual` 2.2 explain all register.

### GPIO
GPIO is the 40 pin on raspi board.

## Mailbox
Mailbox can let ARM communicate with VideoCoreIV GPU.

### Mailbox Conponents
- Mailbox registers
  - accessed by MMIO, Mailbox 0 (check GPU), Mailbox 1 (CPU RW GPU)
- Channels
  - 
- Message
  - [0:3] channel number
  - [4:31] message addr
  - step
    - 


## Extra Reference
- [raspi3-tutorial](https://github.com/bztsrc/raspi3-tutorial/tree/master)
