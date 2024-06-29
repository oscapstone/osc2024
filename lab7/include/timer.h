#pragma once

#include <stdint.h>

#define TIMER_SPEED 3  // default 5

typedef struct timerEntry_t {
  void (*func)(void *);
  void *arg;
  int time;
  struct timerEntry_t *next;
} timerEntry;

void init_timer();

void enable_timer_interrupt();
void disable_timer_interrupt();

void timer_irq_handler();
uint64_t timer_get_uptime();

void timer_add(void (*callback)(void *), void *arg, int duration);
void set_timer(const char *message, int duration);
void set_timeup(int *timeup);
