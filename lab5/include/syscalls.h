#pragma once

#include <stddef.h>

#include "traps.h"

extern void child_ret_from_fork();  // traps.S

int sys_getpid();
size_t sys_uart_read(char *buf, size_t size);
size_t sys_uart_write(const char *buf, size_t size);
int sys_exec(const char *name, trap_frame *tf);
int sys_fork(trap_frame *tf);
void sys_exit(int status);
// void sys_kill(int pid);
// void sys_signal(int signum, void (*handler)());
// void sys_sigkill(int pid, int sig);
void sys_sigreturn(trap_frame *regs);