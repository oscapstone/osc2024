#include "mini_uart.h"
#include "simple_shell.h"


void kernel_main(void)
{
    mini_uart_init();
    mini_uart_puts("Hello, world!\r\n");
    
    simple_shell();
}