#include "alloc.h"
#include "devtree.h"
#include "initrd.h"
#include "irq.h"
#include "mm.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main()
{
    /* Initialization */
    uart_init();
    alloc_init();
    mem_init();
    enable_interrupt();
    timer_enable_interrupt();
    fdt_traverse(initrd_callback);

    /* Shell */
    uart_puts("Welcome!\n");
    run_shell();

    return 0;
}