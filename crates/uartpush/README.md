# `uartpush`

`uartpush` is a utility designed to push the kernel through a UART-connected serial device for loading by [`uartload`](../uartload/).

## Usage

```shell
Usage: uartpush [OPTIONS]

Options:
  -i, --image <IMAGE>          Path to the kernel image to push [default: kernel8.img]
  -d, --device <DEVICE>        Path to the UART serial device [default: /dev/ttyUSB0]
  -b, --baud-rate <BAUD_RATE>  Baud rate to use for the serial device [default: 115200]
  -h, --help                   Print help
```
