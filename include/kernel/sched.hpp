#pragma once

#include "mm/mmu.hpp"
#include "util.hpp"
struct Kthread;

struct __attribute__((__packed__)) Regs {
  uint64_t x19 = 0, x20 = 0, x21 = 0, x22 = 0, x23 = 0, x24 = 0, x25 = 0,
           x26 = 0, x27 = 0, x28 = 0;
  void *fp = 0, *lr = 0, *sp = 0;
  void show() const;
};

extern "C" {
// sched.S
void switch_to_regs(Regs* prev, Regs* next, Kthread* nthread, PT* pgd);
void save_regs(Regs* r);
void load_regs(Regs* r, Kthread* nthread, PT* pgd);
}

void switch_to(Kthread* prev, Kthread* next);

void push_rq(Kthread* thread);
void erase_rq(Kthread* thread);
Kthread* pop_rq();

void push_dead(Kthread* thread);
Kthread* pop_dead();

void kill_zombies();
void schedule_init();
void schedule_lock();
void schedule_unlock();
void schedule();
void schedule_timer();
void schedule_timer_callback(void*);
