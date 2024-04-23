#ifndef __EXCEPTION_H
#define __EXCEPTION_H

void enable_interrupt();
void disable_interrupt();
void exception_entry();
void irq_handler_entry();

void rx_task();
void tx_task();
void timer_task();

void p0_task();
void p1_task();
void p2_task();
void p3_task();
void test_preemption();

#endif