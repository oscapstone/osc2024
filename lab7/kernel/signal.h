
#ifndef SIGNAL_H
#define SIGNAL_H

#include <bsp/traps.h>

#define NSIG 10 

void signal(int signum, void (*handler)());
void sigkill(int pid, int sig);
void do_signal(trap_frame *regs);

#endif 
