#include "alloc.h"
#include "devtree.h"
#include "initrd.h"
#include "irq.h"
#include "mm.h"
#include "sched.h"
#include "shell.h"
#include "syscall.h"
#include "timer.h"
#include "uart.h"

int main()
{
    /* Initialization */
    uart_init();
    alloc_init();
    enable_interrupt();
    timer_init();
    timer_enable_interrupt();
    fdt_traverse(initrd_callback);
    mem_init();
    kthread_init();

    /* Shell */
    uart_puts("Welcome!\n");
    run_shell();

    return 0;
}