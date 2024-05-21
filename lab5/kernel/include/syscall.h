#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define MAX_SYSCALL 7

#include "stdint.h"
#include "exception.h"

int syscall_getpid(trapframe_t *tpf);
size_t syscall_uart_read(trapframe_t *tpf, char buf[], size_t size);
size_t syscall_uart_write(trapframe_t *tpf, const char buf[], size_t size);
int syscall_exec(trapframe_t *tpf, const char *name, char *const argv[]);
int syscall_fork(trapframe_t *tpf);
int syscall_exit(trapframe_t *tpf, int status);
int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox);
int kernel_fork();
int kernel_exec(const char *name, char *const argv[]);
void el0_jump_to_exec();

#endif /* _SYSCALL_H_ */