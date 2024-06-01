#pragma once

#include "base.h"
#include "io/trapFrame.h"

// predefine the struct
struct _CPU_REGS;

struct _TASK;


typedef struct _TASK_SIGNAL
{
    void (*handler)(void);      // function pointer of this signal (in user space)
    void* stack;                // the user stack for this signal
    struct _CPU_REGS* cpu_regs;  // save the current state 
    U64 sp0;                    // save the current sp0_el0
    int count;                  // how many time this signal needed to be handle
    BOOL is_handled;            // whether this signal has been handled
}TASK_SIGNAL;

#define SIGNAL_NUM      10

#define SIGNAL_KILL     9

void signal_check(TRAP_FRAME* trap_frame);
void signal_exit();

void signal_task_init(struct _TASK* task);