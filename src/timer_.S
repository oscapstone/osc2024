#define CORE0_TIMER_IRQ_CTRL 0x40000040 // Core 0 Timers interrupt control: https://datasheets.raspberrypi.com/bcm2836/bcm2836-peripherals.pdf

.global core_timer_enable
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0            // enable

    mrs x0, cntfrq_el0              // get timer frequency
    msr cntp_tval_el0, x0           // set expired time
    
    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str x0, [x1]                    // unmask timer, w0 is 32bit, x0 is 64bit
    ret