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

.globl get_current_time
get_current_time:
    mrs x0, cntpct_el0
    mrs x1, cntfrq_el0
    udiv x0, x0, x1
    ret

.globl get_current_ticks
get_current_ticks:
    mrs x0, cntpct_el0
    ret

.globl get_freq
get_freq:
    mrs x0, cntfrq_el0
    ret

.globl set_core_timer_timeout_ticks
set_core_timer_timeout_ticks:
    msr cntp_tval_el0, x0
    ret

.globl set_core_timer_timeout_secs
set_core_timer_timeout_secs:
    mrs x1, cntfrq_el0
    bl get_freq
    mul x0, x0, x1
    bl set_core_timer_timeout_ticks
    ret

