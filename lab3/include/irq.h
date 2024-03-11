#ifndef IRQ_H
#define IRQ_H

void enable_core_timer();
void core_timer_irq_handler();
void irq_entry();

#endif // IRQ_H