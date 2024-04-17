#ifndef _MULTITASK_H
#define _MULTITASK_H
#include "types.h"

#define USR_STK_SZ (1 << 12)  // 4 KB
#define KER_STK_SZ (1 << 12)  // 4 KB

typedef void* (*start_routine_t)(void*);

typedef struct cpu_context_t {
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
  // fp is x29 and pc is x30
  // frame pointer reg: used to point the end of the last stack frame
  uint64_t fp;
  uint64_t lr;
  // stack pointer reg: used to point the end of the current stack frame
  uint64_t sp;
} cpu_context_t;

typedef enum task_status_t {
  FREE,
  READY,
  DEAD,
} task_status_t;

typedef struct task_struct {
  cpu_context_t cpu_context;
  start_routine_t start_routine;
  task_status_t status;
  struct task_struct* next;
  void* usr_stk;
  void* ker_stk;
} task_struct;

extern void switch_to(task_struct* next);
extern task_struct* get_current_thread();

void init_sched_thread();
task_struct* thread_create(start_routine_t start_routine, void* arg);

#endif