.section ".text"

.global core_timer_enable
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0 // enable
    mov x0, 2
    // mrs x0, cntfrq_el0
    ldr x1, =0x40000040 // timer interrupt control register for core 0
    str w0, [x1] // unmask timer interrupt
    ret

.global core_timer_disable
core_timer_disable:
    mov x0, 0
    msr cntp_ctl_el0, x0 // disable
    ret

.global get_cpu_cycles
get_cpu_cycles:
    mrs x0, cntpct_el0
    ret

.global get_cpu_freq
get_cpu_freq:
    mrs x0, cntfrq_el0
    ret

.global set_timer_asm
set_timer_asm:
    msr cntp_tval_el0, x0
    ret