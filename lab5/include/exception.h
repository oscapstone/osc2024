#ifndef EXCEPTION_H
#define EXCEPTION_H


void enable_interrupt();
void disable_interrupt();
void show_exception_info(int type, unsigned long spsr_el1, unsigned long esr_el1, unsigned long elr_el1);
void test_timer(uint32_t);
// void exception_invalid_handler();
// void except_el0_sync_handler();
// void except_el1_sync_handler();
void timer_handler();
void exception_el1_irq_handler();

#endif