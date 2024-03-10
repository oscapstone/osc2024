#include "headers/mini_uart.h"


void kernel_start()
{
    mini_uart_init();

    mini_uart_puts("Hello from RPI\n");
    while(1)
    {
        mini_uart_putc( mini_uart_getc());
    }// while
    return;
}
