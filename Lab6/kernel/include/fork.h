#ifndef FORK_H
#define FORK_H

#include "sched.h"

#define USR_STK_SZ     (1 << 14)  // 16 KB
#define USR_SIG_STK_SZ (1 << 12)  // 4 KB
#define KER_STK_SZ     (1 << 14)  // 16 KB

#define USR_CODE_ADDR    0x0
#define USR_STK_ADDR     0xffffffffb000
#define USR_SIG_STK_ADDR 0xffffffff7000

#define IO_PM_START_ADDR 0x3C000000
#define IO_PM_END_ADDR   0x40000000

int copy_process(unsigned long clone_flags, void* fn, void* arg1, void* arg2);
int move_to_user_mode(unsigned long pc, unsigned long size);
struct pt_regs* task_pt_regs(struct task_struct* task);

struct pt_regs {
    unsigned long regs[31];
    unsigned long sp;
    unsigned long pstate;  // spsr
    unsigned long pc;      // elr
};

#endif /* FORK_H */
