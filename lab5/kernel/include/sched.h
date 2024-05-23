#ifndef _SCHED_H_
#define _SCHED_H_

#include "list.h"
#include "stdint.h"

#define MAX_PID 32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000
#define SIGNAL_MAX  64

extern void  switch_to(void *curr_context, void *next_context);
extern void* get_current_thread_context();
extern void* set_current_thread_context(void *curr_context);
extern void  store_context(void *curr_context);
extern void  load_context(void *curr_context);

// arch/arm64/include/asm/processor.h - cpu_context
typedef struct thread_context
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
    uint64_t fp;  // x29: base pointer for local variable in stack
    uint64_t lr;  // x30: store return address
    uint64_t sp;  // stack pointer, varys from function calls
} thread_context_t;

enum thread_status
{
    THREAD_RUNNING = 0,
    THREAD_READY,
    THREAD_BLOCKED,
    THREAD_ZOMBIE
};

typedef struct child_node
{
    struct list_head    listhead;
    int64_t             pid;
} child_node_t;

typedef struct thread_struct
{
    struct list_head    listhead;                          	// Freelist node
    thread_context_t 	context;                            // Thread registers
    void*            	code;								// Process itself
    size_t				datasize;		    				// Process size
    int8_t             	status;                          	// Process statement
    int64_t            	pid;                               	// Process ID
    int64_t            	ppid;                               // Parent Process ID
    child_node_t*      	child_list;                         // Child Process List
    char*            	user_stack_base;                   	// User space Stack (Process itself)
    char*            	kernel_stack_base;      			// Kernel space Stack (Kernel syscall)
    char*            	name;                             	// Process name
} thread_t;

int8_t thread_code_can_free(thread_t *thread);
void schedule_timer();
void init_thread_sched();
void _init_create_thread(char *name, int64_t pid, int64_t ppid, void *start);
int64_t wait();
void idle();
void __init();
void init();
void schedule();
void thread_exit();
int8_t has_child(thread_t *thread);
thread_t *thread_create(void *start, char *name);
void dump_run_queue();
void recursion_run_queue(thread_t *root, int64_t level);
void dump_child_thread(thread_t *thread);

void foo();

#endif /* _SCHED_H_ */
