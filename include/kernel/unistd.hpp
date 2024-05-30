#define NR_syscalls 11

#ifndef __SYSCALL
#define __SYSCALL(x, y)
#endif

#define __NR_getpid 0
__SYSCALL(__NR_getpid, sys_getpid)

#define __NR_uartread 1
__SYSCALL(__NR_uartread, sys_uartread)

#define __NR_uartwrite 2
__SYSCALL(__NR_uartwrite, sys_uartwrite)

#define __NR_exec 3
__SYSCALL(__NR_exec, sys_exec)

#define __NR_fork 4
__SYSCALL(__NR_fork, sys_fork)

#define __NR_exit 5
__SYSCALL(__NR_exit, sys_exit)

#define __NR_mbox_call 6
__SYSCALL(__NR_mbox_call, sys_mbox_call)

#define __NR_kill 7
__SYSCALL(__NR_kill, sys_kill)

#define __NR_signal 8
__SYSCALL(__NR_signal, sys_signal)

#define __NR_signal_kill 9
__SYSCALL(__NR_signal_kill, sys_signal_kill)

#define __NR_mmap 10
__SYSCALL(__NR_mmap, sys_mmap)
