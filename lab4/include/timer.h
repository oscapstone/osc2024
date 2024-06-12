#pragma once

#include <stdint.h>

typedef struct __timer_t {
  void (*func)(void *);
  void *arg;
  int time;
  struct __timer_t *next;
} timer_entry;

void enable_timer_interrupt();
void disable_timer_interrupt();
void timer_irq_handler();
uint64_t timer_get_uptime();

void timer_add(void (*callback)(void *), void *arg, int duration);
void set_timer(const char *message, int duration);
