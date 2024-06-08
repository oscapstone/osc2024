#ifndef __THREAD_H
#define __THREAD_H

#include "mm.h"
#include "signal.h"
#include "syscall.h"
#include "vfs.h"

#define NULL 0

struct cpu_context // callee saved
{
    unsigned long long x19;
    unsigned long long x20;
    unsigned long long x21;
    unsigned long long x22;
    unsigned long long x23;
    unsigned long long x24;
    unsigned long long x25;
    unsigned long long x26;
    unsigned long long x27;
    unsigned long long x28;
    unsigned long long fp;
    unsigned long long lr;
    unsigned long long sp;
};

enum task_state
{
    RUNNING,
    IDLE,
    EXIT
};

typedef struct task_struct
{
    struct cpu_context cpu_context; // task context
    struct mm_struct *mm_struct;    // mm strcut
    int id;                         // task id
    int priority;                   // task priority
    void *kstack;                   // stack for kernel
    void *ustack;                   // stack for user
    enum task_state state;          // task state
    int need_sched;                 // Do this task need to yield cpu ?

    int received_signal;                    // the signal that the process received
    int is_default_signal_handler[SIG_NUM]; // check if is the default signal handler
    void (*signal_handler[SIG_NUM])(void);  // signal handler

    struct dentry *pwd; // pwd of this task
    struct task_struct *prev, *next;
} task_struct;

typedef struct task_list
{
    task_struct *head[10];
} task_list;

extern task_list run_queue;

extern task_struct *get_current_task();
extern void switch_mm_irqs_off(unsigned long long *pgd);
extern void switch_to(task_struct *prev, task_struct *next);
extern void user_switch_to(task_struct *next);

void sched_init();

void run_queue_push(task_struct *new_task, int priority);
void run_queue_remove(task_struct *remove_task, int priority);

task_struct *task_create(void (*start_routine)(void), int priority);
void task_exit();

void zombie_reaper();
void kill_zombies();
void schedule();
void context_switch(struct task_struct *next);
void check_need_schedule();
void check_signal(struct ucontext *sigframe);
void do_exec(const char *name, char *const argv[]);

#endif