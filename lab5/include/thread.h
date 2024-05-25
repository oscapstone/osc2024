#ifndef _THREAD_H
#define _THREAD_H


#define T_STACK_SIZE (0x1000) // 2^12 = 4096 = 4KB = 1 page

typedef enum thread_state {
    TASK_RUNNING,
    TASK_WAITING,
    TASK_ZOMBIE,
} thread_state;

// for callee-saved registers
typedef struct callee_reg_t {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long lr;
    unsigned long sp;
} callee_reg_t;

typedef struct thread_t {
    // need to be put as the first variable
    callee_reg_t callee_reg;
    int tid; // thread id
    thread_state state;
    void* user_stack;
    void* kernel_stack;

    // use in queue
    struct thread_t *prev;
    struct thread_t *next;
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

void schedule();
thread_t* create_thread(void (*func)(void));
thread_t* create_fork_thread(void (*func)(void));
thread_t* get_thread_from_tid(int tid);

void kill_thread(int tid);
void kill_zombies();

void thread_init();
void thread_exit();


void idle(); // function for idle thread
void foo();
void thread_test();
void run_fork_test();
void main_fork_test();
void fork_test();

#endif