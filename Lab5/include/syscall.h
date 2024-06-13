#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "exception.h"
#include <stddef.h>

int getpid(trapframe_t *tp);
size_t uartread(trapframe_t *tp, char buf[], size_t size);
size_t uartwrite(trapframe_t *tp, const char buf[], size_t size);
int exec(trapframe_t *tp, const char *name, char *const argv[]);
int fork(trapframe_t *tp);
void exit(trapframe_t *tp, int status);
int syscall_mbox_call(trapframe_t *tp, unsigned char ch, unsigned int *mbox);
void kill(trapframe_t *tp, int pid);
void register_signal(int SIGNAL, void (*handler)());
void signal_kill(int pid, int SIGNAL);
char *get_file_start(char *thefilepath);
unsigned int get_file_size(char *thefilepath);
void sigreturn(trapframe_t *tp);

#endif /* _SYSCALL_H_*/