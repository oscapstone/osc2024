#include "peripheral/timer.h"

.globl enable_core0_timer
enable_core0_timer:
    mov x1, #CNTP_CTL_EL0_ENABLE
    msr cntp_ctl_el0, x1

    mov x1, #nCNTPNSIRQ_IRQ_ENABLE
    ldr x2, =CORE0_TIMER_IRQ_CTRL
    str w1, [x2]
    ret

.globl disable_core0_timer
disable_core0_timer:
    mov x1, #CNTP_CTL_EL0_DISABLE
    msr cntp_ctl_el0, x1

    mov x1, #CORE_TIMER_DISABLE_ALL
    ldr x2, =CORE0_TIMER_IRQ_CTRL
    str w1, [x2]
    ret
