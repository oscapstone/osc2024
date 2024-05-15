#ifndef SCHED_H
#define SCHED_H

#define STACK_SIZE 4096

struct thread_struct {
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
};

enum task_state {
    TASK_RUNNING,
    EXIT_ZOMBIE,
};

struct task_struct {
    struct thread_struct context;
    int pid;
    enum task_state state;
    void *stack;
    void *user_stack;
    struct task_struct *prev;
    struct task_struct *next;
};

typedef struct task_queue_t {
    struct task_struct *front;
    struct task_struct *rear;
} task_queue_t;

extern struct task_struct *get_current();
void kthread_init();
struct task_struct *kthread_create(void (*func)());
void kthread_exit();
void kthread_stop(int pid);
void schedule();
void idle();

void display_run_queue();
void thread_test();

#endif // SCHED_H