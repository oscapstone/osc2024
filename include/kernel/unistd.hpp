#define __NR_syscalls 3

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_getpid 0
__SYSCALL(__NR_getpid, sys_getpid)

#define __NR_uartread 1
__SYSCALL(__NR_uartread, sys_uartread)

#define __NR_uartwrite 2
__SYSCALL(__NR_uartwrite, sys_uartwrite)
