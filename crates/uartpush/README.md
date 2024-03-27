# `uartpush`

`uartpush` is a utility designed to push the kernel through a UART-connected serial device for loading by [`uartload`](../uartload/).

## Usage

```shell
uartpush is a utility designed to push the kernel through a UART-connected serial device for loading by uartload

Usage: uartpush [OPTIONS] --image <IMAGE> --device <DEVICE>

Options:
  -i, --image <IMAGE>          Path to the kernel image to push
  -d, --device <DEVICE>        Path to the UART serial device
  -b, --baud-rate <BAUD_RATE>  Baud rate to use for the serial device [default: 115200]
  -h, --help                   Print help
  -V, --version                Print version
```
