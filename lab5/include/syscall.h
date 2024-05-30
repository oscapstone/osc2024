#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "thread.h"
#include "mini_uart.h"
#include "exception.h"
#include <stddef.h>

int getpid();
size_t uart_read(char buf[], size_t size);
size_t uart_write(const char buf[], size_t size);
int exec(const char* name, char *const argv[]);
void fork(trapframe_t* tf);
void exit(int status);
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);
void signal(int signal, void (*handler)());
void posix_kill(int pid, int signal);
void sigreturn();


int sys_getpid();
int sys_fork();
void sys_exit(int status);
void sys_sigreturn();

#endif