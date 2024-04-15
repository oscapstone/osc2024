#ifndef EXCEPTION_H
#define EXCEPTION_H


void enable_interrupt();
void disable_interrupt();
void test_timer(uint32_t);
void except_handler_c();
void low_irq_heandler_c();

#endif