#include "timer.h"
#include "uart1.h"
#include "utils.h"

#define STR(x) #x
#define XSTR(s) STR(s)

void core_timer_enable(unsigned long long expired_time){
    __asm__ __volatile__(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t" // enable

        "mrs x1, cntfrq_el0\n\t"
        "mul x1, x1, %0\n\t"
        "msr cntp_tval_el0, x1\n\t" // set expired time

        "mov x2, 2\n\t"
        "ldr x1, =" XSTR(CORE0_TIMER_IRQ_CTRL) "\n\t"
        "str w2, [x1]\n\t" // unmask timer interrupt
    :"=r" (expired_time));
}

void core_timer_handler(){
    __asm__ __volatile__("mrs x10, cntpct_el0\n\t");
    register unsigned long long cntpct_el0 asm ("x10");

    __asm__ __volatile__("mrs x11, cntfrq_el0\n\t");
    register unsigned long long cntfrq_el0 asm ("x11");

    uart_puts("## Interrupt - el1_irq ## %d seconds after booting\n", cntpct_el0/cntfrq_el0);

    __asm__ __volatile__(
        "mrs x0, cntfrq_el0\n\t"
        "mov x0, x0, LSL #1\n\t"   //set two second next time
        "msr cntp_tval_el0, x0\n\t"
    );
}
