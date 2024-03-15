#include "timer.h"
#include "uart.h"

void timer_enable_interrupt()
{
    asm volatile("mov x0, 1;"
                 "msr cntp_ctl_el0, x0;"
                 "mrs x0, cntfrq_el0;"
                 "msr cntp_tval_el0, x0;"
                 "mov x0, 2;"
                 "ldr x1, =0x40000040;"
                 "str w0, [x1];");
}

void timer_disable_interrupt()
{
    // TODO: Implement timer_disable_interrupt function
}

void timer_irq_handler()
{
    asm volatile("mrs x0, cntfrq_el0;"
                 "msr cntp_tval_el0, x0;");

    unsigned long long cntpct_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));

    unsigned long long seconds = cntpct_el0 / cntfrq_el0;
    uart_puts("Seconds: ");
    uart_hex(seconds);
    uart_putc('\n');

    unsigned long long wait = cntfrq_el0 * 2;
    asm volatile("msr cntp_tval_el0, %0" ::"r"(wait));
}