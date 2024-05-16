#ifndef __TIMER_H
#define __TIMER_H

#include "timer_heap.h"

#define CORE0_TIMER_IRQ_CTRL 0x40000040

void timer_heap_init();
void set_min_expire();
void add_timer(timer t);
void re_shedule(void *data, int executed_time);

void core_timer_enable();
void core_timer_disable();

extern timer_heap *timer_hp;

#endif
