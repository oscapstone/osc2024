#ifndef _EXCEPTION_H
#define _EXCEPTION_H 

#define CORE0_INTERRUPT_SOURCE 0x40000060

#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)

#define IRQ_PENDING_1_AUX_INT (1<<29)

void exception_entry(void);
void enable_interrupt(void);
void disable_interrupt(void);
void irq_exception_handler_c(void);
void irq_timer_exception(void);

#endif