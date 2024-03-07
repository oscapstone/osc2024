### Basic Exercise 1 - Basic Initialization - 20%

    Initialize rpi3 after booted by bootloader.

- All it’s data is presented at correct memory address.
- The program counter is set to correct memory address.
- The bss segment are initialized to 0.
- The stack pointer is set to a proper address.  

<br/><br/>

### Basic Exercise 2 - Mini UART - 20%

    Follow UART to set up mini UART.

- Rpi3 has 2 different UARTs, mini UART and PL011 UART. In this lab, you need to set up the mini UART.

<br/><br/>

### Basic Exercise 3 - Simple Shell - 20%

    Implement a simple shell supporting the listed commands.

| command   | Description                       |
|-----------|-----------------------------------|
| help      | print all available commands      |
| hello     | print Hello World!                |
| reboot    | reboot the device                 |

- There may be some text alignment issue on screen IO, think about \r\n on both input and output.

<br/><br/>

### Basic Exercise 4 - Mailbox - 20%

    Get the hardware’s information by mailbox and print them, 
    you should at least print board revision and ARM memory base address and size.

<br/><br/>

### Advanced Exercise 1 - Reboot - 30%

    Add a <reboot> command.


```
/* 
    This snippet of code only works on real rpi3, not on QEMU. 
*/

#define PM_PASSWORD 0x5a000000
#define PM_RSTC 0x3F10001c
#define PM_WDOG 0x3F100024

void set(long addr, unsigned int value) {
    volatile unsigned int* point = (unsigned int*)addr;
    *point = value;
}

void reset(int tick) {                 // reboot after watchdog timer expire
    set(PM_RSTC, PM_PASSWORD | 0x20);  // full reset
    set(PM_WDOG, PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reset() {
    set(PM_RSTC, PM_PASSWORD | 0);  // full reset
    set(PM_WDOG, PM_PASSWORD | 0);  // number of watchdog tick
}
```