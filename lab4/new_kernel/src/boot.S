#include "sysregs.h"
.section ".text.boot"

.global _start

_start:
    // bl el2_entry
    // read cpu id, to suspend other CPU
    mrs         x1, mpidr_el1
    and         x1, x1, #3
    cbz         x1, 2f

    // cpu id > 0, stop. to make only one CPU work.

1:
    wfe
    b           1b
    // cpu id == 0
2:
    // set top of stack just before our code (stack grows to a lower address per AAPCS64)
    ldr         x1, = __stack_top
    mov         sp, x1

    // clear bss
    ldr         x1, =__bss_start
    ldr         w2, =__bss_size
3:
    cbz         w2, 4f
    str         xzr, [x1], #8
    sub         w2, w2, #1
    cbnz        w2, 3b

4:
    //should be 8
    mrs         x0, CurrentEL
    //bl          main;
    //b           1b;

.global set_for_el_switch
set_for_el_switch:
    //disable MMU
    ldr         x1, = SCTLR_VALUE_MMU_DISABLED
    msr         sctlr_el1, x1

    // set aarch64 for EL2
    ldr         x1, =HCR_RW
    msr         hcr_el2, x1

    // mask all interrupt, and set EL to EL1h
    ldr         x1, =SPSR_EL2
    msr         spsr_el2, x1

    adr         x1, el1_entry
    msr         elr_el2, x1

    eret
el1_entry:
    mrs         x0, CurrentEL //should be 4

    //set EL1 stack pointer
    ldr         x1, =0x60000
    mov         sp, x1

    //load exception table to VBAR_EL1
    adr         x1, exception_table
    msr         vbar_el1, x1
    bl          main
    b           1b
    //adr x1, exception_table
    //msr vbar_el1, x1

    //mask all interrupt, and set excption to el0
.global el0_enter
el0_enter:
    /*ldr         x1, =SPSR_EL1
    msr         spsr_el1, x1

    adr         x1, el0_entry
    msr         elr_el1, x1

    eret*/

el0_entry:
    //mrs         x25, CurrentEL   //不知道為何不能過 會跳一個什麼0x400 udf #0 到 0x200 udf #0然後死迴圈
    /*AFAIK, that of course is the whole point! EL0 is the unprivileged 'app' mode;
    apps cannot/must not have access to info like this..
    So, my understanding is, write a kernel module to read register values.
    It should work as it runs in EL1, if not EL2. */

    /*ldr         x1, =0x40000
    mov         sp, x1
    bl          main
    b           1b*/
/*
#define CORE0_TIMER_IRQ_CTRL 0x40000040
.global core_timer_enable1
core_timer_enable1:
    mov         x0, 1
    msr         cntp_ctl_el0, x0 // enable
    mrs         x0, cntfrq_el0
    msr         cntp_tval_el0, x0 // set expired time
    mov         x0, 2
    ldr         x1, =CORE0_TIMER_IRQ_CTRL
    str         w0, [x1]// unmask timer interrupt
    ret

.global core_timer_handler1
core_timer_handler1:
    mrs         x0, cntfrq_el0
    msr         cntp_tval_el0, x0
    ret
*/