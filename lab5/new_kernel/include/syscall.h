#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "stdint.h"

int sys_getpid();
size_t sys_uart_read(char buf[], size_t size);
size_t sys_uart_write(const char buf[], size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork();
void sys_exit();
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);

#endif