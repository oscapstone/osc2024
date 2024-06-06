#pragma once

#include "ds/list.hpp"
#include "ds/mem.hpp"
#include "fs/files.hpp"
#include "mm/mm.hpp"
#include "mm/vmm.hpp"
#include "sched.hpp"
#include "signal.hpp"
#include "util.hpp"

constexpr uint64_t KTHREAD_STACK_SIZE = PAGE_SIZE;

enum KthreadStatus {
  kReady = 1,
  kRunning,
  kWaiting,
  kDead,
};

struct Kthread : ListItem<Kthread> {
  Regs regs;

  using fp = void (*)(void*);
  int tid;
  KthreadStatus status = kReady;
  list<Kthread*>::iterator it;
  int exit_code = 0;
  Mem kernel_stack;
  Signal signal;
  VMM vmm;
  Files files;

 private:
  Kthread();
  friend void kthread_init();

 public:
  Kthread(Kthread::fp start, void* ctx, Vnode* cwd);
  Kthread(const Kthread* o);
  Kthread(const Kthread& o) = delete;
  ~Kthread();

  void fix(const Kthread& o, Mem& mem);
  void fix(const Kthread& o, void* faddr, uint64_t fsize);
  void* fix(const Kthread& o, void* ptr);
  void reset_kernel_stack() {
    regs.sp = kernel_stack.end(0x10);
  }
  void vma_print() {
    vmm.vma_print(tid);
  }
};

inline void set_current_thread(Kthread* thread) {
  write_sysreg(TPIDR_EL1, thread);
}
inline Kthread* current_thread() {
  return (Kthread*)read_sysreg(TPIDR_EL1);
}

extern ListHead<Kthread*> kthreads;
void add_list(Kthread* thread);
void del_list(Kthread* thread);
Kthread* find_thread_by_tid(int tid);

void idle();
int new_tid();

void kthread_init();
void kthread_start();
void kthread_wait(int pid);
void kthread_kill(int pid);
void kthread_kill(Kthread* thread);
void kthread_exit(int status);
void kthread_fini();
Kthread* kthread_create(Kthread::fp start, void* ctx = nullptr,
                        Vnode* cwd = root_node);
long kthread_fork();
