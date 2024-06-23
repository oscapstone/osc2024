#ifndef _THREAD_H
#define _THREAD_H


#define T_STACK_SIZE (2 * 0x1000) // 2^12 = 4096 = 4KB = 1 page
#define SIGNAL_NUM 9
#define THREAD_MAX_FD 16

#include <stdint.h>
#include "fs_vfs.h"

extern int is_init_thread;

typedef enum thread_state {
    TASK_RUNNING,
    TASK_WAITING,
    TASK_ZOMBIE,
} thread_state;

// for callee-saved registers
typedef struct callee_reg_t {
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
    uint64_t fp;
    uint64_t lr;
    uint64_t sp;
} callee_reg_t;

typedef struct thread_t {
    // need to be put as the first variable
    callee_reg_t callee_reg;
    int tid; // thread id
    thread_state state;
    void* user_stack;
    void* kernel_stack;
    void* data;
    void* data_size;

    // signal
    void (*signal_handler[SIGNAL_NUM+1])();
    // 0: not waiting, 1: waiting
    int waiting_signal[SIGNAL_NUM+1];
    int is_processing_signal;
    callee_reg_t signal_regs;

    // use in queue
    struct thread_t *prev;
    struct thread_t *next;

    // use in file
    int max_fd;
    file fds[THREAD_MAX_FD];
    vnode *working_dir;
} thread_t;



// defined in context_switch.S
extern void switch_to(thread_t* cur, thread_t* next);
extern thread_t* get_current_thread();

// queue-related
void push(thread_t** head, thread_t* t);
void push_running(thread_t* t);
thread_t* pop(thread_t** head); // pop front
void pop_t(thread_t** head, thread_t* t); // pop given thread
void print_queue(thread_t* head);
void print_running();

void schedule();
thread_t* create_thread(void (*func)(void));
thread_t* create_fork_thread();
thread_t* get_thread_from_tid(int tid);

void kill_thread(int tid);
void kill_zombies();

void thread_init();
void thread_exit();
void thread_wait(int tid);

void idle(); // function for idle thread
void foo();
void thread_test();
void run_fork_test();
void main_fork_test();
void fork_test();

#endif