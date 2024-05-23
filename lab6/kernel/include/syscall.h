#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "exception.h"
#include "stddef.h"
// #define SYSCALL_DEFINE(name) int name(trapframe_t *tpf)


// SYSCALL_DEFINE(getpid);
// SYSCALL_DEFINE(uartread);
// SYSCALL_DEFINE(uartwrite);
// SYSCALL_DEFINE(exec);
// SYSCALL_DEFINE(fork);
// SYSCALL_DEFINE(exit);
// SYSCALL_DEFINE(syscall_mbox_call);
// SYSCALL_DEFINE(kill);
// SYSCALL_DEFINE(signal_register);
// SYSCALL_DEFINE(signal_kill);
// SYSCALL_DEFINE(mmap);
// SYSCALL_DEFINE(signal_reture);
/*
 * the code referenced from https://elixir.bootlin.com/linux/latest/source/include/linux/syscalls.h#L106
 * __MAP - apply a macro to syscall arguments
 * __MAP(n, m, t1, a1, t2, a2, ..., tn, an) will expand to
 *    m(t1, a1), m(t2, a2), ..., m(tn, an)
 * The first argument must be equal to the amount of type/name
 * pairs given.  Note that this list of pairs (i.e. the arguments
 * of __MAP starting at the third one) is in the same format as
 * for SYSCALL_DEFINE<n>/COMPAT_SYSCALL_DEFINE<n>
 */
#define __MAP0(m,...)
#define __MAP1(m,t,a,...) m(t,a)
#define __MAP2(m,t,a,...) m(t,a), __MAP1(m,__VA_ARGS__)
#define __MAP3(m,t,a,...) m(t,a), __MAP2(m,__VA_ARGS__)
#define __MAP4(m,t,a,...) m(t,a), __MAP3(m,__VA_ARGS__)
#define __MAP5(m,t,a,...) m(t,a), __MAP4(m,__VA_ARGS__)
#define __MAP6(m,t,a,...) m(t,a), __MAP5(m,__VA_ARGS__)
#define __MAP(n,...) __MAP##n(__VA_ARGS__)
#define __SC_DECL(t, a)	t a



#define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, name, __VA_ARGS__)
#define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, name, __VA_ARGS__)
#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, name, __VA_ARGS__)
#define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, name, __VA_ARGS__)
#define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, name, __VA_ARGS__)
#define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, name, __VA_ARGS__)
#define SYSCALL_DEFINEx(x, name, ...)                         \
    int sys_##name(__MAP(x,__SC_DECL,__VA_ARGS__))            \

#define SYSCALL_DEFINE(name) int name(trapframe_t *tpf)

#define SYSCALL_TABLE_SIZE 60


SYSCALL_DEFINE1(getpid, trapframe_t*, tpf);
SYSCALL_DEFINE1(uartread, trapframe_t*, tpf);
SYSCALL_DEFINE1(uartwrite, trapframe_t*, tpf);
SYSCALL_DEFINE1(exec, trapframe_t*, tpf);
SYSCALL_DEFINE1(fork, trapframe_t*, tpf);
SYSCALL_DEFINE1(exit, trapframe_t*, tpf);
SYSCALL_DEFINE1(mbox_call, trapframe_t*, tpf);
SYSCALL_DEFINE1(kill, trapframe_t*, tpf);
SYSCALL_DEFINE1(signal_register, trapframe_t*, tpf);
SYSCALL_DEFINE1(signal_kill, trapframe_t*, tpf);
SYSCALL_DEFINE1(mmap, trapframe_t*, tpf);
SYSCALL_DEFINE1(signal_reture, trapframe_t*, tpf);

void init_syscall();
unsigned int get_file_size(char *thefilepath);
char        *get_file_start(char *thefilepath);
typedef struct SYSCALL_TABLE
{
    int (*func)(trapframe_t *tpf);
} SYSCALL_TABLE_T;
#endif /* _SYSCALL_H_*/
