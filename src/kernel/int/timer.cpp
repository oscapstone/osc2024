#include "int/timer.hpp"

#include "int/interrupt.hpp"
#include "int/irq.hpp"

uint64_t freq_of_timer, boot_timer_tick;
ListHead<Timer> timers;

void timer_init() {
  freq_of_timer = read_sysreg(CNTFRQ_EL0);
  boot_timer_tick = get_timetick();
  write_sysreg(CNTP_CTL_EL0, 1);
  timers.init();

  // allow EL0 access physical timer
  auto cntkctl = read_sysreg(CNTKCTL_EL1);
  write_sysreg(CNTKCTL_EL1, cntkctl | 1);
}

void add_timer(timeval tval, void* context, Timer::fp callback, int prio) {
  add_timer(timeval2tick(tval), context, callback, prio);
}
void add_timer(uint64_t tick, void* context, Timer::fp callback, int prio) {
  save_DAIF_disable_interrupt();

  tick += get_timetick();

  auto it = timers.begin();
  for (; it != timers.end(); ++it)
    if (it->tick >= tick)
      break;
  auto is_head = it == timers.begin();
  timers.insert_before(it, new Timer{tick, prio, callback, context});
  if (is_head)
    set_timer_tick(tick);
  if (timers.size() == 1)
    enable_timer();

  restore_DAIF();
}

void timer_enqueue() {
  if (timers.empty()) {
    disable_timer();
    return;
  }

  auto it = *timers.begin();
  timers.erase(timers.begin());

  if (timers.empty())
    disable_timer();
  else
    set_timer_tick(timers.begin()->tick);

  irq_add_task(it->prio, it->callback, it->context);
  delete it;
}
