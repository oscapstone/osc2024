#ifndef _MULTITASK_H
#define _MULTITASK_H
#include "types.h"

#define USR_STK_SZ (1 << 12)  // 4 KB
#define KER_STK_SZ (1 << 12)  // 4 KB

#define PROC_NUM 1024

typedef void (*start_routine_t)(void);

typedef struct cpu_context_t {
  // ARM calling conventions: regs x0 - x18 can be
  // overwritten by the called function.
  // The caller must not assume that the values of those
  // registers will survive after a function call.
  uint64_t x19;
  uint64_t x20;
  uint64_t x21;
  uint64_t x22;
  uint64_t x23;
  uint64_t x24;
  uint64_t x25;
  uint64_t x26;
  uint64_t x27;
  uint64_t x28;
  // frame pointer reg: used to point the end of the last stack frame
  uint64_t fp;  // x29
  uint64_t lr;  // x30
  // stack pointer reg: used to point the end of the current stack frame
  uint64_t sp;
} cpu_context_t;

typedef enum task_state_t {
  THREAD_FREE,
  THREAD_READY,
  THREAD_DEAD,
} task_state_t;

typedef struct task_struct {
  uint64_t pid;
  cpu_context_t cpu_context;
  task_state_t status;
  struct task_struct *prev, *next;
  void* usr_stk;
  void* ker_stk;
  void* data;
  uint32_t data_size;
} task_struct;

typedef struct ts_que {
  task_struct* top;
} ts_que;

typedef struct ts_deque {
  task_struct* head;
  task_struct* tail;
} ts_deque;

extern void switch_to(cpu_context_t* cur, cpu_context_t* to);
extern cpu_context_t* get_current_thread();
extern void set_current_thread(cpu_context_t* cpu_context);

void foo();
void schedule();
void init_sched_thread();
task_struct* get_free_thread();
task_struct* thread_create(start_routine_t start_routine);
void startup_thread_exec(char* file);
void thread_exec_func(start_routine_t start_routine);
void task_exit();
#endif