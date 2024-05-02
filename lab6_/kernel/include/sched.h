#ifndef _SCHED_H_
#define _SCHED_H_

#include "u_list.h"

#define PIDMAX 32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000
#define SIGNAL_MAX  64

extern void  switch_to(void *curr_context, void *next_context);
extern void* get_current();
extern void  store_context(void *curr_context);
extern void  load_context(void *curr_context);

#define SIZE_OF_RUEQUEUE 11

#define IDLE_PRIORITY 100
#define SHELL_PRIORITY 100
#define NORMAL_PRIORITY 50

// arch/arm64/include/asm/processor.h - cpu_context
typedef struct thread_context
{
    unsigned long x19; // callee saved registers: the called function will preserve them and restore them before returning
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;  // base pointer for local variable in stack
    unsigned long lr;  // store return address
    unsigned long sp;  // stack pointer, varys from function calls
    void* pgd;   // use for MMU mapping (user space)

} thread_context_t;

typedef struct thread
{
    list_head_t      listhead;
    thread_context_t context;
    char*            data;
    unsigned int     datasize;
    int              priority;
    int              iszombie;
    int              pid;
    int              isused;
    char*            stack_alloced_ptr;
    char*            kernel_stack_alloced_ptr;
    void             (*signal_handler[SIGNAL_MAX+1])();
    int              sigcount[SIGNAL_MAX + 1];
    void             (*curr_signal_handler)();
    int              signal_is_checking;
    thread_context_t signal_saved_context;
    list_head_t      vma_list;
} thread_t;

extern thread_t    *curr_thread;
extern list_head_t *run_queue;
extern list_head_t *wait_queue;
extern thread_t    threads[PIDMAX + 1];

void schedule_timer(char *notuse);
void init_thread_sched();
void idle();
void add_task_to_runqueue(thread_t *t);
void schedule();
void kill_zombies();
void thread_exit();
thread_t *thread_create(void *start, int priority);
int exec_thread(char *data, unsigned int filesize);

void foo();

#endif /* _SCHED_H_ */
