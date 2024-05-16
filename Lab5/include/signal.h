#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include "exception.h"

#define	SIGKILL		9	/* Kill, unblockable (POSIX).  */

void signal_default_handler();
void check_signal(trapframe_t *tp);
void run_signal(trapframe_t *tp, int signal);
void signal_handler_wrapper();

#endif
