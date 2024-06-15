#ifndef SIGNAL_H
#define SIGNAL_H

#include "sched.h"

#define SIGKILL 9

extern void sig_return(void);
int reg_sig_handler(struct task_struct* task,
                    int SIGNAL,
                    void (*sig_handler)());
int recv_sig(struct task_struct* task, int SIGNAL);
void handle_sig(void);
void do_sig_return(void);
struct pt_regs* task_sig_regs(struct task_struct* task);

#endif /* SIGNAL_H */
