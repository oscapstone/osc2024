.globl irq_vector_init
.global core_timer_enable

irq_vector_init:
	adr	x0, vectors		// load VBAR_EL1 with virtual
	msr	vbar_el1, x0		// vector table address
	ret


#define CORE0_TIMER_IRQ_CTRL 0x40000040
core_timer_enable:
	mov x0, 1
	msr cntp_ctl_el0, x0 // enable timer
	mrs x0, cntfrq_el0
	msr cntp_tval_el0, x0 // set expired time
	mov x0, 2
	ldr x1, =CORE0_TIMER_IRQ_CTRL
	str w0, [x1] // unmask timer interrupt
	ret