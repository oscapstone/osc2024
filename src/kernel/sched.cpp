#include "sched.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"

ListHead<KthreadItem> rq;
void push_rq(Kthread* thread) {
  rq.push_back(thread->item);
}
Kthread* pop_rq() {
  return rq.pop_front()->thread;
}

void kill_zombies() {
  // TODO
}

void schedule_init() {
  rq.init();
}

void schedule() {
  save_DAIF_disable_interrupt();

  auto cur = current_thread();
  switch (cur->status) {
    case KthreadStatus::kReady:
      push_rq(cur);
      break;
    case KthreadStatus::kWaiting:
      // TODO:
      break;
    case KthreadStatus::kDead:
      // TODO:
      break;
  }
  auto nxt = pop_rq();

  restore_DAIF();
  switch_to(cur, nxt);
}
