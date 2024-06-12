#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "exception.h"

void default_signal_handler();
void handler_wrapper();

void check_signal(trapframe_t *tf);
void run_signal(trapframe_t *tf, int signal);

#endif