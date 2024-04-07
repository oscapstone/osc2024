#include "timer.h"
#include "uart.h"

void core_timer_enable()
{
    asm volatile(
        "mov x0, 1\n"
        "msr cntp_ctl_el0, x0\n" // enable
        "mrs x0, cntfrq_el0\n"
        "msr cntp_tval_el0, x0\n" // set expired time
        "mov x0, 2\n"
        "ldr x1, =0x40000040\n"
        "str w0, [x1]\n"); // unmask timer interrupt
}