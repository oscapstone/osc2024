#ifndef INTERRUPT_H
#define INTERRUPT_H

#define CORE0_INT_SRC_GPU (1 << 8)
#define CORE0_INT_SRC_TIMER (1 << 1)

void el0_64_irq_interrupt_handler();
void el1h_irq_interrupt_handler();
#endif