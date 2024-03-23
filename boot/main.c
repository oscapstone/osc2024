#include "uart.h"

void main()
{
    uart_init();

    while (1) {
    uart_puts("you entered=2: ");
        uart_send(uart_getc());
        uart_puts("\n");
    }
}