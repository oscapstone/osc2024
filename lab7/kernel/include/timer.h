#ifndef __TIMER_H
#define __TIMER_H

#include "mmu.h"
#include "timer_heap.h"

#define CORE0_TIMER_IRQ_CTRL (volatile unsigned int *)(VA_START | 0x40000040)
#define CORE0_INTERRUPT_SOURCE (volatile unsigned int *)(VA_START | 0x40000060)

void timer_heap_init();
void set_min_expire();
void add_timer(timer t);
void re_shedule(void *data, int executed_time);
void periodic_timer(void *data, int executed_time);

void core_timer_enable();
void core_timer_disable();

extern timer_heap *timer_hp;

#endif
