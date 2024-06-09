#ifndef	_SCHEDULAR_H
#define	_SCHEDULAR_H

#define USTACK_SIZE 0x10000
#define KSTACK_SIZE 0x10000

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

typedef struct thread_control_block{
    struct thread_control_block* prev;
    struct thread_control_block* next;
    thread_context_t context;
    int pid;
    void* k_stack_allo_base;
    void* u_stack_allo_base;
    char* proc;

} TCB;

extern TCB* running_ring;
extern TCB* zombie_list;
TCB* thread_create(void* proc_start);
void context_output(thread_context_t* context);
void thread_exit();
void idle();
void foo();
void fork_test();

#endif  /*_SCHEDULAR_H */
