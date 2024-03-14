#include "header/uart.h"
#include "header/shell.h"

void main()
{

    uart_init();
    uart_send_string("Type in `help` to get instruction menu!\r\n");
    shell();
}
