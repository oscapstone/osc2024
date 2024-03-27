#include "uart.h"
#include "shell.h"

int get_hardware_info();

void main()
{
    // set up serial console
    uart_init();
    uart_puts("Kernel!\n");
    while(1) {
        shell();
    }
}