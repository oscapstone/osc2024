#ifndef __THREAD_H__
#define __THREAD_H__

#define MAX_SIGNAL 9

#define TASK_RUNNING 0
#define TASK_WATING 1
#define TASK_ZOMBIE 2

#define THREAD_STACK_SIZE 4096

typedef struct{
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
} callee_reg;

typedef struct thread{
  callee_reg regs;
  int tid;
  int state;
  void *stack;
  void *kernel_stack;

  /* signal */
  void (*signal_handler[MAX_SIGNAL+1])();
  int signal_processing;
  int signal_pending;
  callee_reg signal_regs;

  /* queue */
  struct thread *next;
  struct thread *prev;
} thread;

extern void switch_to(thread *current, thread *next);
extern thread* get_current();

void thread_init();
thread *thread_create(void (*func)(void));
void thread_exit();
void thread_kill(int tid);
thread *get_thread(int tid);

void kill_zombies();
void idle();

void schedule();

void thread_test();

#endif