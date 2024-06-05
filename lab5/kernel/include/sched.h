#ifndef _SCHED_H_
#define _SCHED_H_

#include "u_list.h"
#include "signal.h"

#define PIDMAX 32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000

extern void  switch_to(void *curr_context, void *next_context);
extern void* get_current();
extern void load_context(void *curr_context);

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
} thread_context_t;

typedef struct signal_struct
{
    int              lock;                                // Signal Processing Lock
    void             (*handler_table[SIGNAL_MAX])();      // Signal handlers for different signal
    int              pending[SIGNAL_MAX];               // Signal Pending buffer
    thread_context_t saved_context;                       // Store registers before signal handler involving
    void             (*curr_handler)();                   // Allow Signal handler overwritten by others
    char*            stack_base;
}signal_t;

typedef struct thread
{
    list_head_t      listhead;                              // Freelist node
    thread_context_t context;                               // Thread registers
    int              pid;                                   // Process ID
    int              iszombie;                              // Process statement
    int              isused;                                // Freelist node statement
    char*            stack_allocated_base;                  // Process Stack (Process itself)
    char*            kernel_stack_allocated_base;           // Process Stack (Kernel syscall)
    char*            data;                                  // Process itself
    unsigned int     datasize;                              // Process size
    signal_t         signal;                                // Signal info
} thread_t;

void init_thread_sched();
void idle();
void schedule();
void kill_zombies();
void thread_exit();
thread_t *thread_create(void *start);
int exec_thread(char *data, unsigned int filesize);
void schedule_timer();

void foo();

#endif /* _SCHED_H_ */
