#ifndef SIGNAL_H
#define SIGNAL_H

#include "traps.h"

#define NSIG 10 // Number of signals

void signal(int signum, void (*handler)());
void kill(int pid, int sig);
void do_signal(pt_regs *regs);

#endif // SIGNAL_H
