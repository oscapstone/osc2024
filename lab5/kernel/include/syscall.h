#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define MAX_SYSCALL 9

#include "stdint.h"
#include "exception.h"

int sys_getpid(trapframe_t *tpf);
size_t sys_uart_read(trapframe_t *tpf, char buf[], size_t size);
size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size);
int sys_exec(trapframe_t *tpf, const char *name, char *const argv[]);
void run_user_task();
int exec(const char *name, char *const argv[]);
int sys_fork(trapframe_t *tpf);
int fork();
int sys_exit(trapframe_t *tpf, int status);
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox);
int kernel_fork();
int kernel_exec(const char *name, char *const argv[]);

#endif /* _SYSCALL_H_ */