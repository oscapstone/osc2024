#include "uart1.h"
#include "shell.h"

void main()
{
    uart_init();
    start_shell();
}
