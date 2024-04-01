#define CORE0_TIMER_IRQ_CTRL 0x40000040 // core0 timer interrupt control address

.global core_timer_enable
core_timer_enable:
  mov x0, 1
  msr cntp_ctl_el0, x0  // bit0 : timer enabled , bit1 : IMASK won't trigger interrupt
  ret

.global core0_timer_interrupt_enable
core0_timer_interrupt_enable:
  mov x0, 2
  ldr x1, =CORE0_TIMER_IRQ_CTRL         
  str w0, [x1]          // unmask timer interrupt (bit1 : IRQ enable)
  ret

.global core_timer_disable
core_timer_disable:
  mov x0, 0
  msr cntp_ctl_el0, x0  // disable (bit0: timer output signal);
  ret

.global core0_timer_interrupt_disable
core0_timer_interrupt_disable:
  mov x0, 0
  ldr x1, =CORE0_TIMER_IRQ_CTRL 
  str w0, [x1]          // mask timer interrupt
  ret

.global core_timer_handler
core_timer_handler:
  mrs x1, cntfrq_el0    // Counter-timer Frequency register: holds the clock frequency of the system
  mul x1, x0, x1        // set expired time(on a write of this register, CNTP_CVAL_EL0 is set to (CNTPCT_EL0 + timer value)
  msr cntp_tval_el0, x1 // if CNTP_CTL_EL0[0] is 1, then the timer met when (CNTPCT_EL0-CNTP_CVAL_EL0) >= 0
  ret

// cntp_ctl_el0: control register for the el1 physical.
// cntpct_el0: The timer’s current count.
// cntp_cval_el0: A compared timer count. If cntpct_el0 >= cntp_cval_el0, interrupt the CPU core.
// cntp_tval_el0: (cntp_cval_el0 - cntpct_el0). Use it to set an expired timer after the current timer count.