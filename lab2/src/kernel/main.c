#include "malloc.h"
#include "shell.h"
#include "uart.h"

void main(char *arg)
{
    uart_init();
    char *string = simple_malloc(8);

    uart_puts("\x1b[2J\x1b[H");
    uart_puts("Hello, kernel World!\n");

    shell();
}