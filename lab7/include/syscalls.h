#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mbox.h"
#include "traps.h"

#define GET_INFO 0

extern void child_ret_from_fork();  // traps.S

int sys_getpid();
size_t sys_uart_read(char *buf, size_t size);
size_t sys_uart_write(const char *buf, size_t size);
int sys_exec(const char *name, trap_frame *tf);
int sys_fork(trap_frame *tf);
void sys_exit(int status);
int sys_mbox_call(unsigned char ch, unsigned int *mbox);
void sys_sigreturn(trap_frame *regs);

int sys_open(const char *pathname, int flags);
int sys_close(int fd);
int sys_write(int fd, void *buf, size_t count);
int sys_read(int fd, void *buf, size_t count);
int sys_mkdir(const char *pathname, unsigned mode);
int sys_mount(const char *target, const char *filesystem);
int sys_chdir(const char *path);

long sys_lseek64(int fd, long offset, int whence);
int sys_ioctl(int fd, unsigned long request, void *info);
