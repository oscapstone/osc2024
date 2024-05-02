#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "exception.h"
#include <stddef.h>

#define SYSCALL_DEFINE(name) int name(trapframe_t *tpf)
void init_syscall();


SYSCALL_DEFINE(getpid);
SYSCALL_DEFINE(uartread);
SYSCALL_DEFINE(uartwrite);
SYSCALL_DEFINE(exec);
SYSCALL_DEFINE(fork);
SYSCALL_DEFINE(exit);
SYSCALL_DEFINE(syscall_mbox_call);
SYSCALL_DEFINE(kill);
SYSCALL_DEFINE(signal_register);
SYSCALL_DEFINE(signal_kill);
SYSCALL_DEFINE(signal_reture);


unsigned int get_file_size(char *thefilepath);
char *get_file_start(char *thefilepath);

typedef struct SYSCALL_TABLE
{
    int (*func)(trapframe_t *tpf);
} SYSCALL_TABLE_T;

#endif /* _SYSCALL_H_*/
