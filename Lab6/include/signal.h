#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include "thread.h"


void signal_check();
void signal_exec(thread *t, int signum);
void handler_container(void (*handler)());
void default_signal_handler();

#endif