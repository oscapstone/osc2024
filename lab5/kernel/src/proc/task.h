#pragma once

#include "base.h"

typedef U64 pid_t;

typedef struct _CPU_REGS
{
    U64 x19, x20;
    U64 x21, x22;
    U64 x23, x24;
    U64 x25, x26;
    U64 x27, x28;

    // fp,=x29
    // lr,=x30, link register, use for jump to current task pc too
    U64 fp, lr;             

    U64 sp;
}CPU_REGS;

// in other word, process
typedef struct _TASK
{
    CPU_REGS cpu_regs;
    pid_t pid;
    U32 flags;
    int priority;               // the nice value how many time this task can be preempt
    /**
     * If preempt is <= 0 means this task can not be preempted by other task
    */
    int preempt;                // current preempt value
    void* kernel_stack;        // the task process kernel mode stack
}TASK;

#define TASK_CPU_REGS_OFFSET    offsetof(cpu_regs, TASK)

#define TASK_STACK_SIZE         (64 * 1024)

#define TASK_FLAGS_ALLOC        0x01
#define TASK_FLAGS_RUNNING      0x02
#define TASK_FLAGS_KERNEL       0x04

#define TASK_MAX_TASKS          500

typedef struct _TASK_MANAGER {
    U32 count;                                  // current allocated number of task
    U32 running;                                // how many task in running queue
    TASK* running_queue[TASK_MAX_TASKS];        // running queue
    TASK tasks[TASK_MAX_TASKS];
}TASK_MANAGER;

void task_init();
void task_schedule();

void task_run(TASK* task);
TASK* task_create(void* func);

// asm
void task_switch_to(TASK* current, TASK* next);
