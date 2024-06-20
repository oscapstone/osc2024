#ifndef _SCHED_H_
#define _SCHED_H_

#include "u_list.h"

#define PIDMAX 32768
#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000
#define SIGNAL_MAX 64
#define STACK_BASE_OFFSET 0x00

extern void switch_to(void *curr_context, void *next_context);
extern void *get_current();
extern void store_context(void *curr_context);
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
    unsigned long fp; // base pointer for local variable in stack
    unsigned long lr; // store return address
    unsigned long sp; // stack pointer, varys from function calls
    void* pgd;   // use for MMU mapping (user space)
} thread_context_t;

typedef struct thread
{
    list_head_t listhead;                     // Freelist node
    thread_context_t context;                 // Thread registers
    char *data;                               // Process itself
    unsigned int datasize;                    // Process size
    int iszombie;                             // Process statement
    int pid;                                  // Process ID
    int isused;                               // Freelist node statement
    char *stack_alloced_ptr;                  // Process Stack (Process itself)
    char *kernel_stack_alloced_ptr;           // Process Stack (Kernel syscall)
    void (*signal_handler[SIGNAL_MAX + 1])(); // Signal handlers for different signal
    int sigcount[SIGNAL_MAX + 1];             // Signal Pending buffer
    void (*curr_signal_handler)();            // Allow Signal handler overwritten by others
    int signal_is_checking;                     // Signal Processing Lock
    thread_context_t signal_savedContext;     // Store registers before signal handler involving
    list_head_t      vma_list;
} thread_t;

typedef struct execfile
{
    char *data;
    unsigned int filesize;
} execfile;


void schedule_timer(char *notuse);
void thread_sched_init();
void idle();
void schedule();
void kill_zombies();
void thread_exit();
thread_t *thread_create(void *start);
int exec_thread();
void exec_proc();
void exec_wrapper();

void foo();

#endif /* _SCHED_H_ */
