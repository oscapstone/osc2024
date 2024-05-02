#define __NR_syscalls 1

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_getpid 0
__SYSCALL(__NR_getpid, sys_getpid)
