#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdint.h>
#include "../peripherals/utils.h"
#include "../DSLibrary/queue.h"

#define MAX_THREADS 10

// Allocate 64 KB for user stack.
#define USTACK      0x10000
// Allocate 64 KB for kernel stack.
#define KSTACK      0x10000

typedef struct context {
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
    uint64_t kernel_sp;
} context_t;

enum state_t {
    CREATED,
    RUNNABLE,
    RUNNING,
    WAITING,
    DEAD
};

typedef struct thread {
    // Process ID.
    int             pid;
    // Current thread within the thread pool is used or free.
    short           free;
    context_t       context;
    enum state_t    state;
    char*           user_stack_ptr;
    char*           kernel_stack_ptr;
} thread_t;

extern thread_t     thread_pool[MAX_THREADS];
extern thread_t*    cur_thread;
extern Queue        run_queue;
extern Queue        wait_queue;
extern Queue        dead_queue;

extern void         switch_to(context_t* cur_context, context_t* next_context);
extern context_t*   get_current(void);
void                init_thread_pool(void);
void                init_scheduler(void);
void                schedule(void);
thread_t*           thread_create(void*(*start_routine)(void *));
void*               foo(void* arg);
void                idle_thread(void);
void                kill_zombies(void);


#endif