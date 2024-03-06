#include "uart.h"

void main()
{
    uart_init();

    uart_puts("hello, world!\n");
    uart_puts("# help\nhelp\t: print this help menu\nhello\t: print Hello World!\nreboot\t: reboot the device\n");

    while (1) {
        uart_send(uart_getc());
    }
}