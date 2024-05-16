#ifndef __SIGNAL_H
#define __SIGNAL_H

#include "syscall.h"

#define SIG_NUM 10

#define NOSIG 0
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9

void default_SIGKILL_handler();
void do_signal(struct ucontext *sigframe, void (*signal_handler)(void));

#endif