#include "headers/mini_uart.h"


void kernel_start()
{
    mini_uart_init();
    while(1);
}
