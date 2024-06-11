#define NR_syscalls 21

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

#define __NR_open 11
__SYSCALL(__NR_open, sys_open)

#define __NR_close 12
__SYSCALL(__NR_close, sys_close)

#define __NR_write 13
__SYSCALL(__NR_write, sys_write)

#define __NR_read 14
__SYSCALL(__NR_read, sys_read)

#define __NR_mkdir 15
__SYSCALL(__NR_mkdir, sys_mkdir)

#define __NR_mount 16
__SYSCALL(__NR_mount, sys_mount)

#define __NR_chdir 17
__SYSCALL(__NR_chdir, sys_chdir)

#define __NR_lseek64 18
__SYSCALL(__NR_lseek64, sys_lseek64)

#define __NR_ioctl 19
__SYSCALL(__NR_ioctl, sys_ioctl)

#define __NR_sync 20
__SYSCALL(__NR_sync, sys_sync)
