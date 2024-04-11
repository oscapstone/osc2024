#ifndef EXCEPTION_H
#define EXCEPTION_H

void svc_router(unsigned long spsr, unsigned long elr, unsigned long esr);
void print_invalid_entry_message();
void disable_irq();
void enable_irq();
void mini_uart_interrupt_enable();
void irq_router();
#endif