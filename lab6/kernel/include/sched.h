#ifndef SCHED_H
#define SCHED_H

#include "list.h"

#define PIDMAX 32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000
#define SIGNAL_MAX 64

typedef void (*signal_handler_t)(void);

typedef enum thread_status {
    NEW,
    RUNNING,
    DEAD,
} thread_status_t;

// callee-saved reg
// other reg are already on stack
typedef struct thread_context
{
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
    void *ttbr0_el1;
} thread_context_t;

typedef struct thread
{
    /* Run queue, wait queue*/
    list_head_t listhead;

    /* Context*/
    // el0 sp -> user_sp
    // el1 sp -> kernel_sp
    // context sp : sp of this thread
    // context switch may use context sp so it stores kernel_sp
    thread_context_t context;

    /* Data info */ 
    // function or program location
    char *data;
    unsigned int datasize;

    /* Thread status*/
    int pid;
    thread_status_t status;

    /* Stack pointers*/
    // el0 call function
    char *user_sp;
    // el1 call function, load all | save all
    char *kernel_sp;

    /* Signal */
    // registered signal call back function
    signal_handler_t signal_handler[SIGNAL_MAX + 1];
    // registered signal number
    int sigcount[SIGNAL_MAX + 1];
    signal_handler_t curr_signal_handler;
    // prevent nested running signal handler
    // set true when kernel checking signal
    int signal_is_checking;
    // before running handler 
    // save origin context
    thread_context_t signal_saved_context;

    /* VMA */
     list_head_t vma_list;
} thread_t;

// track curr_thread
extern thread_t *curr_thread;
extern list_head_t *run_queue;
extern list_head_t *wait_queue;
extern thread_t threads[PIDMAX + 1];

void schedule_timer(char *s);
void init_thread_sched();
void idle();
void schedule();
void kill_zombies();
void thread_exit();
thread_t *thread_create(void *start, unsigned int filesize);
int exec_thread(char *data, unsigned int filesize);

#endif