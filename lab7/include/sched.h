#ifndef SCHED_H
#define SCHED_H

#include "signal.h"
#include "traps.h"
#include "vfs.h"
#include "vm.h"

#define STACK_SIZE 0x4000

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
}; // TODO: Rename to cpu_context

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
    void (*sighand[NSIG + 1])();
    int sigpending;
    int siglock;
    trap_frame sigframe;
    void *sig_stack;
    void *pgd;
    char cwd[PATH_MAX];
    struct file *fdt[MAX_FD];
    struct task_struct *prev;
    struct task_struct *next;
};

extern struct task_struct *get_current();
struct task_struct *get_task(int pid);
void kthread_init();
struct task_struct *kthread_create(void (*func)());
void kthread_exit();
void kthread_stop(int pid);
void schedule();
void idle();

void display_run_queue();
void thread_test();

#endif // SCHED_H
