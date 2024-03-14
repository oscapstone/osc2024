#include "mini_uart.h"
#include "shell.h"

void kernel_main(void)
{
    uart_init();
    uart_send_string("Welcome to Raspberry Pi 3B+\n");

    shell();

    while (1)
        ;
}
