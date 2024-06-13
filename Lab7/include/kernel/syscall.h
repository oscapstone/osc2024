#ifndef SYSCALL_H
#define SYSCALL_H

#include "kernel/syscall.h"
#include "kernel/exception_hdlr.h"
#include "kernel/thread.h"
#include "kernel/uart.h"
#include "kernel/cpio.h"
#include "kernel/mailbox.h"
#include "kernel/process.h"
#include "kernel/vfs.h"
#include "kernel/framebuffer.h"

extern trap_frame_t *current_tf;
extern void load_context(void *context);
#define NR_SIGNALS 64

int getpid();
// read user input from uart into buf
unsigned int uart_read(char buf[], my_uint64_t size);
// write buf to uart
unsigned int uart_write(const char buf[], my_uint64_t size);
int exec(const char* name, char *const argv[]);
int fork(my_uint64_t stack);
void exit();
int mbox_call(unsigned char ch, unsigned int *mbox);
void kill(int pid);

void sigreg(int SIGNAL, void (*handler)());
void sigkill(int pid, int SIGNAL);
void sigret(void);

// syscall number : 11
int open(const char *pathname, int flags);

// syscall number : 12
int close(int fd);

// syscall number : 13
// remember to return read size or error code
long write(int fd, const void *buf, unsigned long count);

// syscall number : 14
// remember to return read size or error code
long read(int fd, void *buf, unsigned long count);

// syscall number : 15
// you can ignore mode, since there is no access control
int mkdir(const char *pathname, unsigned mode);

// syscall number : 16
// you can ignore arguments other than target (where to mount) and filesystem (fs name)
int mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);

// syscall number : 17
int chdir(const char *path);

// syscall number : 18
// you only need to implement seek set
long lseek64(int fd, long offset, int whence);

// syscall number : 19
int ioctl(int fd, unsigned long request, void *info);

#endif