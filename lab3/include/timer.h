#pragma once

#include <stdint.h>

void timer_enable_interrupt();
void timer_disable_interrupt();
void timer_irq_handler();
uint64_t timer_get_uptime();

void timer_add(void (*callback)(void *), void *arg, int after);
void set_timer(const char *message, int after);
void reset_timeup();
int get_timeup();
void set_timeup();