#ifndef THREAD_H
#define THREAD_H

#include "kernel/allocator.h"
#include "kernel/lock.h"
#include "kernel/uart.h"
#include "kernel/utils.h"
#include "kernel/syscall.h"
#include "kernel/mem.h"

#define THREAD_STK_SIZE 4096
// calee saved registers
typedef struct thread_context{
    unsigned long long x19;
    unsigned long long x20;
    unsigned long long x21;
    unsigned long long x22;
    unsigned long long x23;
    unsigned long long x24;
    unsigned long long x25;
    unsigned long long x26;
    unsigned long long x27;
    unsigned long long x28;
    unsigned long long fp;   //x29, pointed to the bottom of the stack, which is the value of the stack pointer just before the function was called(should be immutable).
    unsigned long long lr;   //x30, but it's refered as PC in some implementation
    unsigned long long sp;
}thread_context_t;


// I considered using list like https://github.com/torvalds/linux/blob/master/include/linux/list.h, but it's actually rely on 'container_of' to get corresponding struct address.
// which is quite complex to implement(Another way is to put that list struct in the first element of thread struct so you can get right address using 'next'). 
// But it's actually not a better solution. So I will use simple linked list instead.

typedef struct thread{
    thread_context_t context;
    char *sp;               // make it char in order to access it byte by byte during fork
    struct thread *next;
    struct thread *prev;
    char *data;
    int data_size;
    int status;             // 1 for running, 0 for waiting, -1 for zombie
    int pid;
}thread_t;



extern thread_t *cur_thread;
extern thread_t *run_queue;
extern thread_t *wait_queue;

extern void switch_to(void *prev, void *next);
extern void* get_current();
extern void fork_return(void);
extern void to_user(void);

void thread_init(void);
thread_t* thread_create(void *fn, void *arg);
void thread_yield(void);
void thread_exit(void);
void kill_zombies(void);
void schedule(void);

void idle_task(void);
void foo(void);

#endif