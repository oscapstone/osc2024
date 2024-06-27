#ifndef SCHED_H
#define SCHED_H

#include <kernel/signal.h>
#include <bsp/traps.h>
#include <kernel/fs/vfs.h>

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
    TASK_STOPPED,
    EXIT_ZOMBIE,
};

struct task_struct {
    struct thread_struct context;
    unsigned long pid;
    // unsigned long parent_pid;
    // unsigned long child_count;
    enum task_state state;
    void *stack;
    void *user_stack;
    void (*sighand[NSIG + 1])();
    int sigpending;
    int sighandling;
    trap_frame sigframe;
    void *sig_stack;
    struct task_struct *prev;
    struct task_struct *next;

    char cwd[256];
    struct fdtable fdtable;
};

typedef struct task_queue_t {
    struct task_struct *front;
    struct task_struct *rear;
} task_queue_t;

extern struct task_struct *get_current();
struct task_struct *get_task(int pid);
void kthread_init();
struct task_struct *kthread_create(void (*func)());
void kthread_exit();
void kthread_stop(int pid);
void schedule();
void idle();
void kill_zombies();

void display_run_queue();
void thread_test();

#endif // SCHED_H
