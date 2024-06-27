#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "exception.h"

void signal_default_handler();
void check_signal(trapframe_t *tpf);
void run_signal(trapframe_t *tpf, int signal);
void signal_handler_wrapper();

#endif /* _SIGNAL_H_ */
