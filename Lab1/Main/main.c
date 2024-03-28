#include "../header/mini_uart.h"
#include "../header/shell.h"

void kernel_main(void)
{
    uart_init();
    uart_send_string("Type in `help` to get instruction menu!\r\n");
    shell();
}
