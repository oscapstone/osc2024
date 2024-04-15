#include "../include/timer.h"
#include "../include/timer_utils.h"
#include <stdint.h>

void enable_timer_interrupt()
{
    /*
        core_timer_enable:
        mov x0, 1
        msr cntp_ctl_el0, x0        // enable
        mrs x0, cntfrq_el0
        msr cntp_tval_el0, x0       // set expired time
        mov x0, 2
        ldr x1, =CORE0_TIMER_IRQ_CTRL
        str w0, [x1]                // unmask timer interrupt
    */
    write_sysreg(cntp_ctl_el0, 1);              // enable cpu timer
    uint64_t frq = read_sysreg(cntfrq_el0);     // read cntfrq_el0 register
    write_sysreg(cntp_tval_el0, frq * 2);       // set expired time
    *CORE0_TIMER_IRQ_CTRL = 2;                  // unmask timer interrupt
}

void disable_timer_interrupt()
{
    write_sysreg(cntp_ctl_el0, 0);              // disable cpu timer
    *CORE0_TIMER_IRQ_CTRL = 0;                  // mask timer interrupt
}

void set_expired_time(uint64_t duration)
{
    unsigned long frq = read_sysreg(cntfrq_el0);
    write_sysreg(cntp_tval_el0, frq * duration); // set expired time
}

uint64_t get_current_time()
{
    /* You can get the seconds after booting 
       from the count of the timer(cntpct_el0) and the frequency of the timer(cntfrq_el0).
    */
    uint64_t frq = read_sysreg(cntfrq_el0);
    uint64_t current_count = read_sysreg(cntpct_el0);
    return (uint64_t)(current_count / frq);    
}