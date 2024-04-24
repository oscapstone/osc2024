#ifndef __TIMER_H__
#define __TIMER_H__

#include <kernel/bsp_port/irq.h>
#include <lib/stdlib.h>

void set_timeout(void (*callback)(void *), void *arg, int after);
void timer_irq_handler();

#endif 
