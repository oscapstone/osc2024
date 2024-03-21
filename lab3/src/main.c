#include "alloc.h"
#include "devtree.h"
#include "initrd.h"
#include "irq.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main()
{
    uart_init();
    alloc_init();

    enable_interrupt();
    timer_enable_interrupt();

    fdt_traverse(initrd_callback);
    uart_puts("Welcome!\n");
    run_shell();

    // uart_enable_interrupt();
    // uart_async_write("Test UART Async Write\n");
    // for (int i = 0; i < 1000000000; i++)
    //     ;
    // char buffer[256];
    // uart_async_read(buffer, 256);
    // uart_puts(buffer);

    return 0;
}