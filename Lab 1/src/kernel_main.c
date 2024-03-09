#include "mini_uart.h"
#include "simple_shell.h"


void kernel_main(void)
{
    mini_uart_init();
    mini_uart_puts("Hello, NYCU OSC 2024!\r\n");
    
    simple_shell();
}