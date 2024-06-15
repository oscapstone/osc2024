#ifndef _SYSCALL_H
#define _SYSCALL_H
#define MAX_SYSCALL 12

#include "exception.h"
#include "stdint.h"


#define CALL_SYSCALL(syscall_num) \
	do                            \
	{                             \
		asm volatile(             \
			"mov x8, %0\n\t"      \
			"svc 0\n\t"           \
			:                     \
			: "r"(syscall_num)    \
			: "x8");              \
	} while (0)

enum syscall_num
{
	SYSCALL_GETPID = 0,
	SYSCALL_UART_READ,
	SYSCALL_UART_WRITE,
	SYSCALL_EXEC,
	SYSCALL_FORK,
	SYSCALL_EXIT,
	SYSCALL_MBOX_CALL,
	SYSCALL_KILL,
	SYSCALL_SIGNAL_REGISTER,
	SYSCALL_SIGNAL_KILL,
	SYSCALL_SIGNAL_RETURN,
	SYSCALL_LOCK_INTERRUPT,
	SYSCALL_UNLOCK_INTERRUPT,
	SYSCALL_MAX
};

int sys_getpid(trapframe_t *tpf);
size_t sys_uart_read(trapframe_t *tpf, char buf[], size_t size);
size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size);
int sys_exec(trapframe_t *tpf, const char *name, char *const argv[]);
int sys_fork(trapframe_t *tpf);
int sys_exit(trapframe_t *tpf, int status);
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox);
int sys_kill(trapframe_t *tpf, int pid);

#endif