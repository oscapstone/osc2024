#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "thread.h"

void check_and_run_signal();
void exec_signal(thread_t* t, int signal);
void default_signal_handler();
void handler_warper(void (*handler)());
#endif // _SIGNAL_H