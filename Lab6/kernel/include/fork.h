#ifndef FORK_H
#define FORK_H

#include "sched.h"

int copy_process(unsigned long clone_flags, void* fn, void* arg);
int move_to_user_mode(unsigned long pc);
struct pt_regs* task_pt_regs(struct task_struct* task);

struct pt_regs {
    unsigned long regs[31];
    unsigned long sp;
    unsigned long pstate;  // spsr
    unsigned long pc;      // elr
};

#endif /* FORK_H */
