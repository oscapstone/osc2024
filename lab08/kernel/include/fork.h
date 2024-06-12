#ifndef __FORK_H__
#define __FORK_H__

// int copy_process(unsigned long fn, unsigned long arg);
#include "schedule.h"

int copy_process(
	unsigned long flags,
	unsigned long fn, 
	unsigned long arg,
	unsigned long stack);
int move_to_user_mode(unsigned long pc);

struct pt_regs {
	unsigned long regs[31];
	unsigned long sp;
	unsigned long pc;
	unsigned long pstate;
};

struct pt_regs * task_pt_regs(struct task_struct *tsk);
void kp_user_mode(unsigned long func);

#define PSR_MODE_EL0t    0x00000000;

#endif