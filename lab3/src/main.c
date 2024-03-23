#include "alloc.h"
#include "devtree.h"
#include "initrd.h"
#include "irq.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main()
{
    /* Initialization */
    uart_init();
    alloc_init();
    enable_interrupt();
    timer_enable_interrupt();
    fdt_traverse(initrd_callback);

    uart_async_write("[INFO] Test the UART async write function\n");

    /* Shell */
    uart_puts("Welcome!\n");
    run_shell();

    /* Test the UART async read function */
    // uart_enable_rx_interrupt();
    // for (int i = 0; i < 1000000000; i++)
    //     ;
    // char buffer[256];
    // uart_async_read(buffer, 256);
    // uart_puts(buffer);
    // while (1)
    //     ;

    return 0;
}