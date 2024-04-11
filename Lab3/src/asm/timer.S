// save general registers to stack
.macro save_regs
    sub sp, sp, 32 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
.endm

// load general registers from stack
.macro load_regs
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm

//enable or disable an IRQ or FIQ interrupt. p.13 BCM2836 cpu MEMORY
#define CORE0_TIMER_IRQ_CTRL 0x40000040

.global core_timer_enable
core_timer_enable:
    mov x0, 1
    msr cntp_ctl_el0, x0    //cntp_ctl_el0 = 1 Timer enable https://developer.arm.com/documentation/ddi0595/2021-03/AArch64-Registers/CNTP-CTL-EL0--Counter-timer-Physical-Timer-Control-register?lang=en

    //This register is provided so that software can discover the frequency of the system counter
    mrs x0, cntfrq_el0  //x0 = cntfrq_el0 = Clock frequency https://developer.arm.com/documentation/ddi0601/2024-03/AArch64-Registers/CNTFRQ-EL0--Counter-timer-Frequency-Register
    mov x1, 2
    mul x0, x0, x1  //x0 = Clock frequency * 2
    msr cntp_tval_el0, x0           // set expired time
                                    //Holds the timer value for the EL1 physical timer. https://developer.arm.com/documentation/ddi0595/2021-12/AArch64-Registers/CNTP-TVAL-EL0--Counter-timer-Physical-Timer-TimerValue-register
    //CNTP_CVAL_EL0 = CNTPCT_EL0 + cntp_tval_el0 (current time + time value)
    //when CNTPCT_EL0 >= CNTP_CVAL_EL0 and cntp_ctl_el0[1]=0 => interrupt
    //(current time >= compare value)

    mov x0, 2
    ldr x1, =CORE0_TIMER_IRQ_CTRL
    str x0, [x1]                    // unmask timer 
                                    //Non-secure physical timer event IRQ enable
                                    //Secure physical timer event IRQ disable
    ret     //Return from subroutine, branches unconditionally to an address in a register, with a hint that this is a subroutine return.
            //https://developer.arm.com/documentation/dui0802/b/A64-General-Instructions/RET

.global core_timer_disable
core_timer_disable:
    //mrs x0, cntp_ctl_el0    //x0 = cntp_ctl_el0
    //mov x1, 1
    //eor x1, x1, x1                  //x1 = 0
    //mov x1, 0
    //and x0, x0, x1
    mov x1, 0
    msr cntp_ctl_el0, x1    //Timer disable

    ldr x2, =CORE0_TIMER_IRQ_CTRL
    //ldr x0, [x2]    //x0 = CORE0_TIMER_IRQ_CTRL
    //mov x1, 2
    //eor x1, x1, x1                  // not x1
    mov x1, 0
    //and x0, x0, x1
    str x1, [x2]                    // disable timer interrupt
    ret

.global timer_handler
timer_handler:
    save_regs

    bl  timer_router
    
    load_regs
    eret