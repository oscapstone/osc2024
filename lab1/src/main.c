#include "uart1.h"
#include "shell.h"

void main()
{
    uart_init();
    uart_flush_FIFO();
    start_shell();
}
