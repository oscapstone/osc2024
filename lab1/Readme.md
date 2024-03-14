# 2024 NYCU OSC LAB1 - Hello World

### linker.ld

* This file is to tell linker how to combine all object files to an executable or image.

```c
SECTIONS 
{
    . = 0x80000;	     						// set PC to 0x80000
    .text.boot : { *(.text.boot) }// include all .text.boot section
    .text :  { *(.text) }					// include all code that is not in others section
    .rodata : { *(.rodata) }			// include read-only data
    .data : { *(.data) }					// include global data
    . = ALIGN(0x8);								// align to multiple of 8
    bss_begin = .;								// set current PC to represent begin of bss
    .bss : { *(.bss*) } 					// include global data that is not initalized
    bss_end = .;									// set current PC to represent end of bss
}
```

### uart.c

<img src=./gpfsel1.png width=70% >

* register offset : https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf

### Mailbox

* ARM CPU is able to configure peripherals such as clock rate and framebuffer by calling VideoCoreIV(GPU) routines.  Mailboxes are the communication bridge between them.
