#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_enable_interrupt();
void timer_disable_interrupt();
void timer_irq_handler();
uint64_t timer_get_uptime();

void timer_add(void (*callback)(void *), void *arg, int after);
void set_timeout(const char *message, int after);

#endif // TIMER_H