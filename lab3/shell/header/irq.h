#ifndef IRQ_H
#define IRQ_H
#include "mailbox.h"
#define IRQ_PENDING_1 ((volatile unsigned int *)(MMIO_BASE + 0xB204))
#define CORE0_INTERRUPT_SOURCE ((volatile unsigned int *)(0x40000060))

void except_handler_c();
void irq_except_handler_c();
void enable_interrupt();
void disable_interrupt();
#endif