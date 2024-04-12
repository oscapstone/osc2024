#include "timer.hpp"

#include "interrupt.hpp"
#include "irq.hpp"

uint64_t freq_of_timer, boot_timer_tick, us_tick;
bool show_timer = true;
int timer_cnt = 0;
Timer* timer_head = nullptr;
int timer_delay = 0;

void timer_init() {
  freq_of_timer = read_sysreg(CNTFRQ_EL0);
  us_tick = freq_of_timer / (int)1e6;
  boot_timer_tick = get_timetick();
  set_timer_tick(freq_of_timer);
  write_sysreg(CNTP_CTL_EL0, 1);
}

void add_timer(timeval tval, void* context, Timer::fp callback, int prio) {
  add_timer(timeval2tick(tval), context, callback, prio);
}
void add_timer(uint64_t tick, void* context, Timer::fp callback, int prio) {
  save_DAIF();
  disable_interrupt();

  tick += get_timetick();

  Timer** nptr = &timer_head;
  bool is_head = true;
  for (auto it = timer_head; it; it = it->next)
    if (it->tick <= tick) {
      nptr = &it->next;
      is_head = false;
    } else {
      break;
    }
  *nptr = new Timer{tick, prio, callback, context, *nptr};
  if (is_head)
    set_timer_tick(timer_head->tick - get_timetick());
  if (++timer_cnt == 1)
    enable_timer();

  restore_DAIF();
}

void timer_enqueue() {
  if (timer_cnt == 0) {
    disable_timer();
    return;
  }

  timer_pop(it, timer_head);

  if (--timer_cnt == 0)
    disable_timer();
  else
    set_timer_tick(timer_head->tick - get_timetick());

  irq_add_task(it->prio, it->callback, it->context);
  delete it;
}
