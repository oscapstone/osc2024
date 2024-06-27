
#ifndef syscall_h
#define syscall_h

#include <bsp/traps.h>
#include <lib/stddef.h>

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

int sys_open(trap_frame *tf, const char *pathname, int flags);
int sys_close(trap_frame *tf, int fd);
long sys_write(trap_frame *tf, int fd, const void *buf, unsigned long count);
long sys_read(trap_frame *tf, int fd, void *buf, unsigned long count);
long sys_mkdir(trap_frame *tf, const char *pathname, unsigned mode);
long sys_mount(trap_frame *tf, const char *src, const char *target,
               const char *filesystem, unsigned long flags, const void *data);
long sys_chdir(trap_frame *tf, const char *path);
long sys_lseek64(trap_frame *tf, int fd, long offset, int whence);
long sys_ioctl(trap_frame *tf, int fb, unsigned long request, void *info);

#endif
