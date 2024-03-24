#include "mbox.h"
#include "mini_uart.h"
#include "shell.h"

void kernel_main(void)
{
    uart_init();
    uart_send_string("*************************\r\n");
    uart_send_string("Hello from raspi!\r\n");
    uart_send_string("*************************\r\n");
    while (1)
    {
        char command[100];
        shell_input(command);
        shell_option(command);
    }
}
