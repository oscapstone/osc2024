#ifndef __EXCEPTION_H
#define __EXCEPTION_H

void enable_interrupt();
void disable_interrupt();
void exception_entry();
void irq_handler_entry();

#endif