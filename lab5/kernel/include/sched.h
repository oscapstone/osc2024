#ifndef	_SCHED_H_
#define	_SCHED_H_

#include "list.h"
#include "stdint.h"

#define PIDMAX      32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000
#define SIGNAL_MAX  64

extern void  switch_to(void *curr_context, void *next_context);
extern void* get_current();
extern void  store_context(void *curr_context);
extern void  load_context(void *curr_context);

// arch/arm64/include/asm/processor.h - cpu_context
typedef struct thread_context {
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
} thread_context_t;

enum thread_status {
    THREAD_RUNNING = 0,
    THREAD_READY,
    THREAD_BLOCKED,
    THREAD_ZOMBIE
};

typedef struct thread {
    struct list_head    listhead;                              // Freelist node
    thread_context_t    context;                               // Thread registers
    void*               code;                                  // Process itself
    size_t              datasize;                              // Process size
    int8_t              status;                                // Process statement
    int64_t             pid;                                   // Process ID
    int64_t             ppid;                                  // Parent Process ID
    char*               user_stack_base;                       // User space Stack (Process itself)
    char*               kernel_stack_base;                     // Kernel space Stack (Kernel syscall)
    char*               name;                                  // Process name
    // signal_t            signal;                                // Signal struct
    // child_node_t*       child_list;                            // Child Process List
} thread_t;


#endif /*_SCHED_H_*/