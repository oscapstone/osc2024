#ifndef _MULTITASK_H
#define _MULTITASK_H
#include "multitask.h"
#include "syscall_.h"
#include "types.h"
#include "vfs.h"
#include "vm.h"
#include "vm_macro.h"

#define USR_STK_SZ (1 << 14)      // 16 KB
#define USR_SIG_STK_SZ (1 << 12)  // 4 KB
#define KER_STK_SZ (1 << 14)      // 16 KB

#define USR_CODE_ADDR 0x0
#define USR_STK_ADDR 0xffffffffb000
#define USR_SIG_STK_ADDR 0xffffffff7000

#define IO_PM_START_ADDR 0x3C000000
#define IO_PM_END_ADDR 0x40000000

#define PROC_NUM 1024

#define current_thread get_current_thread()

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
  THREAD_RUNNING,
} task_state_t;

typedef struct task_struct_node {
  struct task_struct_node *prev, *next;
  struct task_struct* ts;
} task_struct_node;

typedef struct task_struct {
  cpu_context_t cpu_context;
  mm_struct mm;
  uint64_t pid;
  cpu_context_t sig_cpu_context;
  task_state_t status;
  task_struct_node ts_node;
  void* usr_stk;
  void* ker_stk;
  void* sig_stk;
  void* data;
  uint32_t data_size;
  sig_handler_func cur_exec_sig_func;
  sig_handler_t sig_handlers[SIG_NUM];
  uint8_t sig_is_check;
  vnode* cwd;
  file* fdtable[MAX_FD];
} task_struct;

typedef struct ts_que {
  task_struct_node* top;
} ts_que;

typedef struct ts_deque {
  task_struct_node* head;
  task_struct_node* tail;
} ts_deque;

extern void switch_to(task_struct* cur, task_struct* to);
extern void store_cpu_context(cpu_context_t* context);
extern void load_cpu_context(cpu_context_t* context);
extern task_struct* get_current_thread();
extern void set_current_thread(task_struct* cpu_context);
extern void set_current_pgd(uint64_t pgd);

void ready_que_del_node(task_struct* thread);
void ready_que_push_back(task_struct* thread);
task_struct* ready_que_pop_front();
void idle_task();
void foo();
void schedule();
void init_sched_thread();
task_struct* get_free_thread();
void user_thread_exec();
task_struct* thread_create(start_routine_t start_routine);
void startup_thread_exec(char* file);
void task_exit();
#endif