#include "shell.h"
#include "uart.h"

void main(char *arg)
{
    uart_init();
    uart_puts("\nHello, kernel World!\n");

    shell();
}