#include "mini_uart.h"
#include "shell.h"

__attribute__((noreturn))
void kernel_main(void)
{
    uart_init();
    uart_send_string("Hello, world!\r\n\0");

    shell();
}
