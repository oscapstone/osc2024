#include "kernel/gpio.h"
#include "kernel/uart.h"
#include "kernel/shell.h"
#include "kernel/cpio.h"
#include "kernel/allocator.h"
#include "kernel/dtb.h"
// this one need to be outside any function
char *cpio_addr;
char *cpio_end;
extern void* _dtb_addr;

void main(void)
{
    uart_init();
    uart_puts("Hello, world! 312552025\r\n");
    /*show every char typed*/
    /*while (1) {
        uart_putc(uart_getc());
    }*/
    void *el;
    uart_b2x_64((unsigned long long)&_start);
    uart_putc('\n');
    uart_b2x_64((unsigned long long)&__end);
    uart_putc('\n');

    fdt_traverse(initramfs_callback);

    asm volatile(
        "mrs %[var1], CurrentEL;"
        : [var1] "=r" (el)    // Output operands
    );

    uart_puts("Current Exception Level: ");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("Set boot timer to 0\n");
    boot_timer_flag = 0;
    //uart_irq_on();
    startup_init();

    my_shell();
}