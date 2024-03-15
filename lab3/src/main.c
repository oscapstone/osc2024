#include "alloc.h"
#include "uart.h"
#include "shell.h"
#include "devtree.h"
#include "initrd.h"
#include "timer.h"

int main()
{
    uart_init();
    alloc_init();
    timer_enable_interrupt();
    fdt_traverse(initrd_callback);
    uart_puts("Welcome!\n");
    run_shell();
    return 0;
}