#include "uart.h"
#include "loader.h"

void main()
{
    uart_init();

    uart_puts("=======UART Bootloader v0.1.0=======\n");

    load_kernel();
}