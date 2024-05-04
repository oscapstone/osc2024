#pragma once

#include <cstdint>

#include "ds/list.hpp"
#include "mm/mm.hpp"
#include "util.hpp"

constexpr uint64_t KTHREAD_STACK_SIZE = PAGE_SIZE;

struct __attribute__((__packed__)) Regs {
  uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27, x28;
  void *fp, *lr, *sp;
  void init() {
    x19 = x20 = x21 = x22 = x23 = x24 = x25 = x26 = x27 = x28 = 0;
    fp = lr = sp = 0;
  }
};

struct KthreadItem : ListItem {
  struct Kthread* thread;
  KthreadItem(Kthread* th) : ListItem{}, thread{th} {}
};

enum class KthreadStatus {
  kReady,
  kWaiting,
  kDead,
};

struct Kthread {
  Regs regs;

  using fp = void (*)(void);
  int tid;
  KthreadStatus status;
  int exit_code;
  char *kernel_stack = nullptr, *user_text = nullptr, *user_stack = nullptr;
  KthreadItem* item;

  void init(Kthread::fp start);
  int alloc_user_text_stack(uint64_t text_size, uint64_t stack_size);
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
void kthread_exit(int status);
void kthread_fini();
Kthread* kthread_create(Kthread::fp start);
