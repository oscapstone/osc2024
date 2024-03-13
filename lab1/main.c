#include "uart.h"
#include "shell.h"

void main()
{
    // init
    uart_init();
    uart_send_string("Type help command\n");
    // interact with console
    shell();
}
