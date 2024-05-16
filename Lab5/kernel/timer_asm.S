#define CORE0_TIMER_IRQ_CTRL 0x40000040

core_timer_enable:
   mov x0, 1
   // Control register for the physical timer on the current core.
   msr cntp_ctl_el0, x0 // enable

   // cntfrq_el0 holds the frequency in Hz at which the system
   // counter increments.
   mrs x0, cntfrq_el0
   msr cntp_tval_el0, x0 // set expired time
   mov x0, 2
   ldr x1, =CORE0_TIMER_IRQ_CTRL

   // Stores the value from "x0"(which is the "w0" register, the 32-bit view of "x0").
   // Writing 2 into the register unmasks the timer interrupt.
   str w0, [x1] // unmask timer interrupt


// Set the timer to expire a second later.
core_timer_handler:
  mrs x0, cntfrq_el0
  msr cntp_tval_el0, x0