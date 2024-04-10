#pragma once

#include "timeval.hpp"

extern uint64_t freq_of_timer, boot_timer_tick, us_tick;
extern bool show_timer;

uint64_t get_timetick();
timeval tick2timeval(uint64_t tick);
uint64_t timeval2tick(timeval tval);

void core_timer_enable();
void set_timer_tick(uint64_t tick);
timeval get_current_time();

struct Timer {
  using fp = void (*)(void*);
  uint64_t tick;
  fp callback;
  void* context;
  Timer* next;
  void call() {
    return callback(context);
  }
};

extern Timer* timer_head;

void add_timer(timeval tval, void* context, Timer::fp callback);
void add_timer(uint64_t tick, void* context, Timer::fp callback);

void timer_handler();
