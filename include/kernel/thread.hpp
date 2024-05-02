#pragma once

#include <cstdint>

#include "ds/list.hpp"
#include "util.hpp"

struct __attribute__((__packed__)) Regs {
  uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
  void *fp, *lr, *sp;
  void init() {
    x19 = x20 = x21 = x22 = x23 = x24 = x25 = x26 = x27 = x28 = 0;
    fp = lr = sp = 0;
  }
};

struct ThreadItem : ListItem {
  struct Kthread* thread;
  ThreadItem(Kthread* th) : ListItem{}, thread{th} {}
};

struct Kthread {
  Regs regs;

  using fp = void (*)(void);
  int tid;
  bool dead = false;
  char* stack;
  void init(Kthread::fp start);
  ThreadItem* item;
};

inline void set_current_thread(Kthread* thread) {
  write_sysreg(TPIDR_EL1, thread);
}
inline Kthread* current_thread() {
  return (Kthread*)read_sysreg(TPIDR_EL1);
}

void idle();
int new_tid();

void kthread_init();
void kthread_start();
void kthread_fini();
Kthread* kthread_create(Kthread::fp start);
