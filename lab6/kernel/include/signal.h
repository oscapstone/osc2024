#ifndef SIGNAL_H
#define SIGNAL_H

#define SIGKILL_NO 9

#define USER_SIG_WRAPPER_VIRT_ADDR_ALIGNED 0xffffffff9000L

#include "syscall.h"
#include "sched.h"
#include "malloc.h"

void signal_default_handler();
void check_signal(trapframe_t *tpf);
void run_signal(trapframe_t* tpf,int signal);
void signal_handler_wrapper();

#endif