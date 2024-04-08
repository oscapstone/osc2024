#ifndef __TIMER_H
#define __TIMER_H

#include "timer_heap.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040

void timer_heap_init();
void set_min_expire();
void add_timer(timer t);
void setTimeout(char *message, int seconds);
void print_message(void *data, int executed_time);
void periodic_timer(void *data, int executed_time);

void core_timer_enable();
void core_timer_disable();

extern heap *hp;

#endif
