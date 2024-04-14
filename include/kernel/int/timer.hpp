#pragma once

#include "board/timer.hpp"
#include "ds/timeval.hpp"

struct Timer {
  using fp = void (*)(void*);
  uint64_t tick;
  int prio;
  fp callback;
  void* context;
  Timer* next;
};

extern uint64_t freq_of_timer, boot_timer_tick, us_tick;
extern int timer_cnt;
extern Timer* timer_head;
// cmds/timer.cpp
extern bool show_timer;
extern int timer_delay;

#define timer_pop(name, head) \
  auto name = head;           \
  head = name->next;          \
  name->next = nullptr

#define get_timetick() read_sysreg(CNTPCT_EL0)

inline timeval tick2timeval(uint64_t tick) {
  uint32_t sec = tick / freq_of_timer;
  uint32_t usec = tick % freq_of_timer / us_tick;
  return {sec, usec};
}

inline uint64_t timeval2tick(timeval tval) {
  return tval.sec * freq_of_timer + tval.usec * us_tick;
}

void timer_init();

inline void enable_timer() {
  set_core0_timer_irq_ctrl(true, 1);
}
inline void disable_timer() {
  set_core0_timer_irq_ctrl(false, 1);
}

#define set_timer_tick(tick) write_sysreg(CNTP_TVAL_EL0, tick)

inline uint64_t get_current_tick() {
  return get_timetick() - boot_timer_tick;
}
inline timeval get_current_time() {
  return tick2timeval(get_current_tick());
}

void add_timer(timeval tval, void* context, Timer::fp callback, int prio = 0);
void add_timer(uint64_t tick, void* context, Timer::fp callback, int prio = 0);

void timer_enqueue();
