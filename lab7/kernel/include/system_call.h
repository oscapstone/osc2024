#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H

#include "exception.h"
#include <stddef.h>

typedef void (*handler) (void);

int do_getpid();
size_t do_uart_read(char buf[], size_t size);
size_t do_uart_write(char buf[], size_t size);
int do_exec(char* name, char *argv[]);
int do_fork(trapframe_t*);
void do_exit();
int do_mbox_call(unsigned char ch, unsigned int *mbox);
void do_kill(int pid);
void do_signal(int, handler);
void do_sigkill(int, int);

void from_el1_to_fork_test();
void simple_fork_test();

// for debug purpose
void get_sp();
void output_sp(void*);

void im_fineee();

#define O_CREAT 00000100
int do_open(const char *pathname, int flags);
int do_close(int fd);
long do_write(int fd, const void *buf, unsigned long count);
long do_read(int fd, void *buf, unsigned long count);
int do_mkdir(const char *pathname, unsigned mode);
int do_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int do_chdir(const char *path);

long do_lseek64(int fd, long offset, int whence);
int do_ioctl();

#endif
