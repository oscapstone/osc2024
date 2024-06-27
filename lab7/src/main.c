#include "devtree.h"
#include "initrd.h"
#include "irq.h"
#include "mm.h"
#include "proc.h"
#include "shell.h"
#include "syscall.h"
#include "timer.h"
#include "uart.h"
#include "vfs.h"

int main()
{
    /* Initialization */
    uart_init();
    mem_init();
    timer_init();
    enable_interrupt();
    fdt_traverse(initrd_callback);
    kthread_init();
    vfs_init();

    /* Shell */
    uart_puts("Welcome!\n");
    run_shell();

    return 0;
}
