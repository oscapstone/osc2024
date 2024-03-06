#include "kernel/gpio.h"
#include "kernel/uart.h"
#include "kernel/shell.h"

void main(void)
{
    uart_init();
    uart_puts("Hello, world! 312552025\r\n");
    /*show every char typed*/
    /*while (1) {
        uart_putc(uart_getc());
    }*/
    my_shell();
}