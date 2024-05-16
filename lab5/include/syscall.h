#ifndef SYSCALL_H
#define SYSCALL_H

#include <stddef.h>

#include "traps.h"

int sys_getpid();
size_t sys_uart_read(char *buf, size_t size);
size_t sys_uart_write(const char *buf, size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork(trap_frame *tf);
void sys_exit();
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);
void sys_signal(int signum, void (*handler)());
void sys_sigkill(int pid, int sig);
void sys_sigreturn(trap_frame *regs);
void run_fork_test();

#endif // SYSCALL_H