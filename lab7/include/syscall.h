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
int sys_open(const char *pathname, int flags);
int sys_close(int fd);
long sys_write(int fd, const void *buf, unsigned long count);
long sys_read(int fd, void *buf, unsigned long count);
int sys_mkdir(const char *pathname, unsigned mode);
int sys_mount(const char *src, const char *target, const char *filesystem,
              unsigned long flags, const void *data);
int sys_chdir(const char *path);
long sys_lseek64(int fd, long offset, int whence);
int sys_ioctl(int fd, unsigned long request, void *info);
void run_fork_test();

#endif // SYSCALL_H
