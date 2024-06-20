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
    unsigned long pgd; // used for user space MMU mapping
} thread_context_t;

typedef struct thread_control_block{
    struct thread_control_block* prev;
    struct thread_control_block* next;
    thread_context_t context;
    int pid;
    void* k_stack_allo_base; //address of kernel stack base in kernel space
    void* u_stack_allo_base; //address of user stack base in kernel space
    char* proc;//address of proc base in kernel space
    unsigned int proc_size; //default value is 0

} TCB;

extern TCB* running_ring;
extern TCB* zombie_list;
TCB* thread_create(void* proc_start,unsigned int proc_size);
void context_output(thread_context_t* context);
void thread_exit();
int kill_zombies();
void idle();
void foo();
void fork_test();

#endif  /*_SCHEDULAR_H */
