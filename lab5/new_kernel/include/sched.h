#ifndef _SCHED_H_
#define _SCHED_H_

#include "stdint.h"
#include "utility.h"

#define MAX_PID 32768

extern void switch_to(void *curr_context, void *next_context);
extern void *get_current_thread_context();
extern void *set_current_thread_context(void *curr_context);
extern void store_context(void *curr_context);
extern void load_context(void *curr_context);

typedef struct cpu_context
{
    uint64_t x19; // callee saved registers: the called function will preserve them and restore them before returning
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp; // x29: base pointer for local variable in stack
    uint64_t lr; // x30: store return address
    uint64_t sp; // stack pointer, varys from function calls
} cpu_context_t;

typedef enum thread_status
{
    THREAD_IS_RUNNING = 0,
    THREAD_IS_BLOCKED,
    THREAD_IS_ZOMBIE
} thread_status_t;

typedef struct child_node
{
    list_head_t list_head;
    int64_t pid;
}child_node_t;


typedef struct thread_struct
{
    list_head_t list_head;
    cpu_context_t context;
    void *code;
    thread_status_t status;
    size_t datasize;
    int64_t pid;
    int64_t ppid;
    child_node_t*       child_list;                            // Child Process List
    char *user_stack_base;
    char *kernel_stack_base;
    char *name;

} thread_t;

// Lab 5 Basic 1
void init_thread_sched();
void foo();

void schedule();
void idle();
void thread_exit();

#endif
