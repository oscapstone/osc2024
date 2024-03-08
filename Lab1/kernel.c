#include "include/mini_uart.h"
#include "include/shell.h"

void kernel_main(void)
{
	uart_init();
    shell();

    // char str[20] = "Hello, world!\r\n";

    // uart_send_string(str);
}
