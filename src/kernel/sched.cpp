#include "sched.hpp"

#include "int/interrupt.hpp"
#include "io.hpp"
#include "thread.hpp"

void Regs::show() const {
  kprintf("=== Regs ===\n");
  kprintf("x19\t= 0x%lu\n", x19);
  kprintf("x20\t= 0x%lu\n", x20);
  kprintf("x21\t= 0x%lu\n", x21);
  kprintf("x22\t= 0x%lu\n", x22);
  kprintf("x23\t= 0x%lu\n", x23);
  kprintf("x24\t= 0x%lu\n", x24);
  kprintf("x25\t= 0x%lu\n", x25);
  kprintf("x26\t= 0x%lu\n", x26);
  kprintf("x27\t= 0x%lu\n", x27);
  kprintf("x28\t= 0x%lu\n", x28);
  kprintf("FP \t= %p\n", fp);
  kprintf("LR \t= %p\n", lr);
  kprintf("SP \t= %p\n", sp);
  kprintf("------------------\n");
}

void switch_to(Kthread* prev, Kthread* next) {
  switch_to_regs(&prev->regs, &next->regs);
}

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
