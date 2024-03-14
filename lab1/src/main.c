#include "shell.h"
#include "uart.h"

void main()
{
    uart_init();
    uart_puts("\nHello, kernel World!\n");

    shell();
}