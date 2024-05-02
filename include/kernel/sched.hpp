#pragma once

#include "ds/list.hpp"
#include "mm/mm.hpp"
#include "util.hpp"

#define STACK_SIZE PAGE_SIZE

struct __attribute__((__packed__)) Regs {
  uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
  void *fp, *lr, *sp;
  void init() {
    x19 = x20 = x21 = x22 = x23 = x24 = x25 = x26 = x27 = x28 = 0;
    fp = lr = sp = 0;
  }
};

struct ThreadItem : ListItem {
  struct Thread* thread;
  ThreadItem(Thread* th) : ListItem{}, thread{th} {}
};

struct Thread {
  Regs regs;

  using fp = void (*)(void);
  int tid;
  bool dead = false;
  char* stack;
  void init(Thread::fp start);
  ThreadItem* item;
};

extern "C" {
// sched.S
void switch_to(Thread* prev, Thread* next);
}

inline void set_current_thread(Thread* thread) {
  write_sysreg(TPIDR_EL1, thread);
}
inline Thread* current_thread() {
  return (Thread*)read_sysreg(TPIDR_EL1);
}

void push_rq(Thread* thread);
Thread* pop_rq();

int new_tid();
void run_thread();
Thread* create_thread(Thread::fp start);

void idle();
void init_thread();
void kill_zombies();
void schedule_init();
void schedule();
