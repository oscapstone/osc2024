#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "exception.h"
#include "stddef.h"

int    getpid(trapframe_t *tpf);
size_t uartread(trapframe_t *tpf, char buf[], size_t size);
size_t uartwrite(trapframe_t *tpf, const char buf[], size_t size);
int    exec(trapframe_t *tpf, const char *name, char *const argv[]);
int    fork(trapframe_t *tpf);
void   exit(trapframe_t *tpf, int status);
int    syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox);
void   kill(trapframe_t *tpf, int pid);
void   signal_register(int signal, void (*handler)());
void   signal_kill(int pid, int signal);
void*  mmap(trapframe_t *tpf, void *addr, size_t len, int prot, int flags, int fd, int file_offset);
void   sigreturn(trapframe_t *tpf);


unsigned int get_file_size(char *thefilepath);
char        *get_file_start(char *thefilepath);

#endif /* _SYSCALL_H_*/
