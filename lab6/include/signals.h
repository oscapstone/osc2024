#pragma once

#include "traps.h"

#define NSIG 10  // Number of signals
#define SIG_STACK (USER_STACK - STACK_SIZE)

extern void sigreturn();  // Defined in traps.S

void signal(int signum, void (*handler)());
void signal_kill(int pid, int sig);
void do_signal(trap_frame *regs);
