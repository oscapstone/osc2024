#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "thread.h"
#include "exception.h"
#include <stddef.h>

int sys_getpid();
size_t sys_uartread(char buf[], size_t size);
size_t sys_uartwrite(char buf[], size_t size);
int sys_exec(const char *name, char *const argv[]);
int sys_fork(trapframe *tf);
void sys_exit();
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_kill(int pid);
void sys_signal(int signum, void (*handler)());
void posix_kill(int pid, int signum);
void sys_sigreturn();
void *sys_mmap(void *addr, unsigned long len, int prot, int flags, int fd, int file_offset);
int sys_open(const char *pathname, int flags);
int sys_close(int fd);
long sys_write(int fd, const void *buf, size_t count);
long sys_read(int fd, void *buf, size_t count);
int sys_mkdir(const char *pathname, unsigned mode);
int sys_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int sys_chdir(const char *path);
int sys_lseek64(int fd, long offset, int whence);
int sys_ioctl(int fd, unsigned long request, void* arg);

/* Wrapper of system call */
int getpid();
int fork();
void exit();
void sigreturn();

/* For test */
void jump_to_el0();
void fork_test();
void sys_fork_test();

#endif