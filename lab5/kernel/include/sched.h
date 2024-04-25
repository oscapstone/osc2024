#ifndef	_SCHED_H_
#define	_SCHED_H_

#include "list.h"
#include "stdint.h"

#define MAX_PID     32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000
#define SIGNAL_MAX  64

extern void  switch_to(void *curr_context, void *next_context);
extern void* get_current();
extern void* set_current(void *curr_context);
extern void  store_context(void *curr_context);
extern void  load_context(void *curr_context);

// arch/arm64/include/asm/processor.h - cpu_context
typedef struct cpu_context {
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
    uint64_t fp;  // x29: base pointer for local variable in stack
    uint64_t lr;  // x30: store return address
    uint64_t sp;  // stack pointer, varys from function calls
}cpu_context_t;

enum thread_status {
    THREAD_RUNNING = 0,
    THREAD_READY,
    THREAD_BLOCKED,
    THREAD_ZOMBIE
};

typedef struct thread {
    struct list_head    listhead;                               // Freelist node
    cpu_context_t       context;                                // Thread registers
    void*               code;                                   // Process itself
    int                 priority;                               // Process priority
    size_t              datasize;                               // Process size
    int8_t              status;                                 // Process statement
    int64_t             pid;                                    // Process ID
    char*               user_stack_base;                        // User space Stack (Process itself)
    char*               kernel_stack_base;                      // Kernel space Stack (Kernel syscall)
    char*               name;
    // signal_t            signal;                                // Signal struct
    // child_node_t*       child_list;                            // Child Process List
} thread_t;

void            init_thread_sched();
thread_t*       _init_thread_0(char* name, int64_t pid, void *start);
thread_t*       thread_create(void *start, char* name); 
void            schedule();
void            schedule_timer();
void            idle();
void            exec_thread(char *data, unsigned int filesize);
void            dump_thread_info(thread_t* t);


#endif /*_SCHED_H_*/