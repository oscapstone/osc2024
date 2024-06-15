#ifndef SCHED_H
#define SCHED_H

#include "bool.h"
#include "def.h"
#include "list.h"

#define current_task    (get_current_task())
#define current_context (current_task->cpu_context)

#define THREAD_STACK_SIZE 0x2000ULL
#define PF_KTHREAD        0x2
#define NR_SIGNAL         10

typedef enum {
    TASK_RUNNING,
    TASK_WAITING,
    TASK_STOPPED,
    TASK_INIT,

} task_state_t;

typedef int pid_t;


struct cpu_context {
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
    unsigned long sp;
    unsigned long pc;
};

struct task_struct {
    struct cpu_context cpu_context;
    task_state_t state;
    long counter;
    long priority;
    long preempt_count;
    struct list_head list;
    pid_t pid;
    void* kernel_stack;
    void* user_stack;
    void* prog;
    void* sig_stack;
    void(*sig_handler[NR_SIGNAL]);
    bool sig_handling;
    long sig_pending;
    unsigned long flags;
};

#define INIT_SIGNAL                                                \
    {                                                              \
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL \
    }

#define INIT_TASK                                                           \
    {                                                                       \
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, TASK_RUNNING, 0, 1, 0,     \
            {NULL, NULL}, 0, NULL, NULL, NULL, NULL, INIT_SIGNAL, false, 0, \
            PF_KTHREAD                                                      \
    }

extern void cpu_switch_to(struct cpu_context* prev, struct cpu_context* next);
extern struct task_struct* get_current_task(void);
extern void set_current_task(struct task_struct* task);
extern void ret_from_fork(void);

int sched_init(void);
struct task_struct* create_task(long prioriy, long preempt_count);
void add_task(struct task_struct* task);
void kill_task(struct task_struct* task);
struct task_struct* find_task(int pid);
void exit_process(void);
void delete_task(struct task_struct* task);
void kill_zombies(void);
void schedule(void);
void timer_tick(char* UNUSED(msg));
void preempt_disable(void);
void preempt_enable(void);
void switch_to(struct task_struct* next);

#endif /* SCHED_H */
