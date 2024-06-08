#ifndef __SCHED_H
#define __SCHED_H

#include "uart.h"
#include "stdint.h"
#include "signal.h"

#define NR_TASKS 16
#define KSTACK_SIZE 4096
#define KSTACK_TOP (KSTACK_SIZE)
#define USTACK_SIZE 4096
#define USTACK_TOP (USTACK_SIZE)

#ifndef PAGE_SIZE
#define PAGE_SIZE               (1 << 12) // 4KB
#endif // PAGE_SIZE

#define current get_current()

#define USER_TASK_PRIORITY 10

/* Used in tsk->state*/
#define TASK_RUNNING            0x00
#define TASK_INTERRUPTIBLE      0x01
#define TASK_UNINTERRUPTIBLE    0x02
#define TASK_ZOMBIE             0x04
#define TASK_STOPPED            0x08

/* Used in tsk->exit_state*/
#define EXIT_SUCCESS            0x000
#define EXIT_DEAD               0x010
#define EXIT_ZOMBIE             0x020

/* cpu context, in linux :thread_struct.cpu_context */
struct task_state_segment {
    uint64_t x19;
    uint64_t x20;
    uint64_t x21;
    uint64_t x22;
    uint64_t x23;
    uint64_t x24;
    uint64_t x25;
    uint64_t x26;
    uint64_t x27;
    uint64_t x28;
    uint64_t fp; // x29, frame pointer
    uint64_t lr; // x30, link register
    uint64_t sp; // sp_el1
    uint64_t sp_backup; // for signal handling
};

struct task_struct {
    unsigned int task_id;
    unsigned int state;
    long priority;
    long counter;

    int pending; // pending signal
    int process_sig; // signal that is being processed
    struct sighandler sighand;

    int exit_state;
    struct task_state_segment tss; // because context switch occurs in kernel mode,sp are in el1 (sp_el1);
};

extern struct task_struct task_pool[NR_TASKS];
extern char kstack_pool[NR_TASKS][KSTACK_SIZE];
extern char ustack_pool[NR_TASKS][USTACK_SIZE];
extern int num_running_task;

/* Get the task_struct of current task. */
extern struct task_struct *get_current(void);
/* Be called when doing context switch. */
extern void switch_to(struct task_state_segment *prev, struct task_state_segment *next);
/* Restore context after signal handling */
extern void sig_restore_context(struct task_state_segment *tsk_tss);
/* Put the pointer of task_struct into tpidr_el1*/
extern void update_current(struct task_struct *next);

void task_init(void);
void sched_init(void);
void context_switch(struct task_struct *next);
int find_empty_task(void);
int privilege_task_create(void (*func)(), long priority);
void schedule(void);

#endif // __SCHED_H