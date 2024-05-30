#ifndef _SHED_H_
#define _SHED_H_

#include "list.h"

#define PIDMAX 32768
#define USTACK_SIZE 0x10000 // user stack size
#define KSTACK_SIZE 0x10000 // kernel stack size

extern void switch_to(void *cur, void *next);
extern void *get_current();
extern void store_context(void *cur);
extern void load_context(void *cur);

#define SIZE_OF_RUNQUEUE 11

#define IDLE_PRIORITY 2
#define SHELL_PRIORITY 2
#define NORMAL_PRIORITY 1

typedef struct cpu_context {
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
    unsigned long fp; // x29, store the frame pointer
    unsigned long lr; // x30, store the return address
    unsigned long sp; // x31, store the stack pointer
} cpu_context_t;

typedef struct thread {
    struct list_head listhead;
    cpu_context_t cpu_context;
    int priority;
    char *data;
    unsigned int datasize;
    int zombie;
    int pid;
    int used;
    char *stack_ptr;
    char *kstack_ptr;
} thread_t;

void schedule();
void thread_schedule_init();
void idle();
void add_task_thread(thread_t *t);
void thread_exit();
thread_t *thread_create(void *start, int priority);
void schedule_timer(char *notuse);
void kill_zombies();
int exec_thread(char *data, unsigned int datasize);
void run_user_process();

void foo();
void _delay(unsigned int count);

#endif