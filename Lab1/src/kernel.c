#include "mini_uart.h"

__attribute__((noreturn))
void kernel_main(void)
{
    uart_init();
    uart_send_string("Hello, world!\r\n\0");

    while (1)
        uart_send(uart_recv());
}
