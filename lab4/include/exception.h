#ifndef EXCEPTION_H
#define EXCEPTION_H


void enable_interrupt();
void disable_interrupt();
void test_timer(uint32_t);
void exception_invalid_handler();
void except_el0_sync_handler();
void except_el1_sync_handler();
void timer_handler();
void exception_el1_irq_handler();

#endif