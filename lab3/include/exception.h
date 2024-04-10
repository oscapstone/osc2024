#ifndef _EXECPTION_H_
#define _EXECPTION_H_

#define CORE0_INTERRUPT_SOURCE 0x40000060

#define INTERRUPT_SOURCE_CNTPNSIRQ (1<<1)
#define INTERRUPT_SOURCE_GPU (1<<8)

#define IRQ_PENDING_1_AUX_INT (1<<29)

void el1_interrupt_enable();
void el1_interrupt_disable();

void exception_handler_c();
void irq_exception_handler_c();

void irq_timer_exception();

#endif