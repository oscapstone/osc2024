#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "exception.h"

int getpid(trapframe_t *tf);
unsigned int uart_read(trapframe_t *tf, char buf[], unsigned int size);
unsigned int uart_write(trapframe_t *tf, const char buf[], unsigned int size);
int exec(trapframe_t *tf, const char *name, char *const argv[]);
int fork(trapframe_t *tf);
void exit(trapframe_t *tf);
int mbox_call(trapframe_t *tf, unsigned char ch, unsigned int *mbox);
void kill(trapframe_t *tf, int pid);

char *cpio_file_start(char *file);
unsigned int cpio_file_size(char *file);

#endif