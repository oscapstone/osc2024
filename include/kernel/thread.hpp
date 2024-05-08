#pragma once

#include <cstdint>

#include "ds/list.hpp"
#include "ds/mem.hpp"
#include "mm/mm.hpp"
#include "sched.hpp"
#include "util.hpp"

constexpr uint64_t KTHREAD_STACK_SIZE = PAGE_SIZE;

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
  KthreadStatus status = KthreadStatus::kReady;
  int exit_code = 0;
  Mem kernel_stack, user_text, user_stack;
  KthreadItem* item;

 private:
  Kthread();
  friend void kthread_init();

 public:
  Kthread(Kthread::fp start);
  Kthread(const Kthread& o);
  void fix(const Kthread& o, void* faddr, uint64_t fsize);
  int alloc_user_text_stack(uint64_t text_size, uint64_t stack_size);
  void reset_kernel_stack() {
    regs.sp = kernel_stack.addr + KTHREAD_STACK_SIZE - 0x10;
  }
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
long kthread_fork();
