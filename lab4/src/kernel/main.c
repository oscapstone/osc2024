#include "dtb.h"
#include "exception.h"
#include "malloc.h"
#include "memory.h"
#include "shell.h"
#include "timer.h"
#include "uart.h"

void main(char *arg)
{
    // pass by x21 reg
    register unsigned long long x21 asm("x21");
    arg = (char *)x21;

    uart_init();
    fdt_traverse(initramfs_callback, arg);

    core_timer_enable();
    core_timer_interrupt_enable();
    set_core_timer_interrupt_permanent();
    uart_interrupt_enable();
    asm volatile("msr DAIFClr, 0xf");

    init_allocator();

    // uart_puts("\x1b[2J\x1b[H");
    uart_puts("Hello, kernel World!\n");
    uart_puts("DTB base: ");
    uart_hex((unsigned long)arg);

    shell();
}