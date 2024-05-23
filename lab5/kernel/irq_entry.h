#ifndef __IRQ_ENTRY_H__
#define __IRQ_ENTRY_H__

#define EL1_IRQ_TIMER_PRIORITY 0x1
#define EL1_IRQ_UART_PRIORITY 0x2

// exception entrypoint for exception.S
void el1_irq_entry(int type, unsigned long spsr, unsigned long elr,
                   unsigned long esr);

void default_exception_entry(int type, unsigned long spsr, unsigned long elr,
                             unsigned long esr);

void el0_irq_entry(int type, unsigned long spsr, unsigned long elr,
                   unsigned long esr);

#endif
