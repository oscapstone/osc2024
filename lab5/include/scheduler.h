#pragma once

#include "signals.h"
#include "traps.h"

struct context_struct {
  unsigned long x19;
  unsigned long x20;
  unsigned long x21;
  unsigned long x22;
  unsigned long x23;
  unsigned long x24;
  unsigned long x25;
  unsigned long x26;
  unsigned long x27;
  unsigned long x28;
  unsigned long fp;
  unsigned long lr;
  unsigned long sp;
};

enum thread_state {
  RUNNING,
  DEAD,
};

typedef struct thread_struct_t {
  struct context_struct context;  // callee-saved (don't move)
  int pid;
  enum thread_state state;
  void *stack;
  void *user_stack;

  // Signal handling
  void (*sig_handlers[NSIG + 1])();  // Signal handlers
  int sig_reg;                       // Pending signals
  int sig_busy;                      // Handling a signal
  trap_frame *sig_tf;                // tf Saved before signal handling
  void *sig_stack;                   // Stack for signal handling

  struct thread_struct_t *prev;
  struct thread_struct_t *next;
} thread_struct;

// Defined in scheduler.S
extern void switch_to(thread_struct *prev, thread_struct *next);
extern thread_struct *get_current();  // get tpidr_el1, current thread_struct

thread_struct *get_thread_by_pid(int pid);
void sched_init();

thread_struct *kcreate_thread(void (*func)());
void list_tcircle();

void schedule();

void kill_current_thread();
void kill_thread_by_pid(int pid);
void idle();