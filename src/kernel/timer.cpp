#include "timer.hpp"

#include "board/timer.hpp"
#include "interrupt.hpp"

uint64_t freq_of_timer, boot_timer_tick, us_tick;
bool show_timer = true;

uint64_t get_timetick() {
  return read_sysreg(CNTPCT_EL0);
}

timeval tick2timeval(uint64_t tick) {
  uint32_t sec = tick / freq_of_timer;
  uint32_t usec = tick % freq_of_timer / us_tick;
  return {sec, usec};
}

uint64_t timeval2tick(timeval tval) {
  return tval.sec * freq_of_timer + tval.usec * us_tick;
}

void core_timer_enable() {
  write_sysreg(CNTP_CTL_EL0, 1);
  freq_of_timer = read_sysreg(CNTFRQ_EL0);
  us_tick = freq_of_timer / (int)1e6;
  boot_timer_tick = get_timetick();
  set_timer_tick(freq_of_timer);
  set_core0_timer_irq_ctrl(true, 1);
}

void set_timer_tick(uint64_t tick) {
  write_sysreg(CNTP_TVAL_EL0, tick);
}

timeval get_current_time() {
  return tick2timeval(get_timetick() - boot_timer_tick);
}

Timer* timer_head = nullptr;

void add_timer(timeval tval, void* context, Timer::fp callback) {
  add_timer(timeval2tick(tval), context, callback);
}
void add_timer(uint64_t tick, void* context, Timer::fp callback) {
  tick += get_timetick();
  disable_interrupt();
  Timer** nptr = &timer_head;
  for (auto it = timer_head; it; it = it->next)
    if (it->tick <= tick)
      nptr = &it->next;
    else
      break;
  *nptr = new Timer{
      tick,
      callback,
      context,
      *nptr,
  };
  set_timer_tick(timer_head->tick - get_timetick());
  enable_interrupt();
}

void timer_handler() {
  while (timer_head != nullptr and timer_head->tick <= get_timetick()) {
    timer_head->call();
    auto it = timer_head;
    timer_head = it->next;
    delete it;
  }
  if (timer_head != nullptr)
    set_timer_tick(timer_head->tick - get_timetick());
}
