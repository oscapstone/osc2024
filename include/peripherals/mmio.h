#ifndef MMIO_H
#define MMIO_H

#define MMIO_BASE 0x3F000000  // 0x3B400000 ~ 0x3F000000 is for VideoCore GPU

#endif

/*
All peripherals communicate in memory with CPU. Each has it's dedicated memory
address starting from 0x3F000000.
0x3F003000 - System Timer 0x3F00B000 - Interrupt controller
0x3F00B880 - VideoCore mailbox
0x3F100000 - Power management
0x3F104000 - Random Number Generator
0x3F200000 - General Purpose IO controller
0x3F201000 - UART0 (serial port, PL011)
0x3F215000 - UART1 (serial port, AUX mini UART)
0x3F300000 - External Mass Media Controller (SD card reader)
0x3F980000 - Universal Serial Bus controller
*/