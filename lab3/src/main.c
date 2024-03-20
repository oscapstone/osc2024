#include "alloc.h"
#include "devtree.h"
#include "initrd.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

int main()
{
    uart_init();
    alloc_init();
    // timer_enable_interrupt();
    fdt_traverse(initrd_callback);
    uart_puts("Welcome!\n");
    run_shell();
    return 0;
}