#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();
    uart_puts("Start Bootloader\n");
    while(1) {
        shell();
    }
}