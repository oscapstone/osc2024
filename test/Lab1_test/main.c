#include "uart.h"
#include "shell.h"

void main()
{
    // set up serial console
    uart_init();

    // say hello
    uart_puts("Hello World!\n");

    shellStart();
}