#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define MAX_SYSCALL 12

#include "stdint.h"
#include "stddef.h"
#include "exception.h"

/**
 * @brief Jump to user space with specific settings.
 *
 * This macro will store address of wrapper function to curr_thread->context.lr.
 *
 * @param wrapper Function pointer to the wrapper function to be called.
 * @param dest Destination address to jump to in user space.
 * @param user_sp Pointer to the user stack.
 * @param kernel_sp Pointer to the kernel stack.
 */
#define JUMP_TO_USER_SPACE(wrapper, dest, user_sp, kernel_sp)                                          \
	do                                                                                                 \
	{                                                                                                  \
		DEBUG("Jump to user space\n");                                                                 \
		curr_thread->context.lr = (uint64_t)wrapper;                                                   \
		if (kernel_sp != NULL)                                                                         \
			asm volatile("mov sp, %0\n\t" ::"r"(kernel_sp)); /* set kernel stack pointer */            \
		if (user_sp != NULL)                                                                           \
			asm volatile("msr sp_el0, %0\n\t" ::"r"(user_sp)); /* el0 stack pointer for el1 process */ \
		el1_interrupt_disable();                                                                       \
		asm volatile(                                                                                  \
			"msr tpidr_el1, %0\n\t" /* Hold the \"kernel(el1)\" thread structure information */        \
			"msr elr_el1, %1\n\t"	/* When el0 -> el1, store return address for el1 -> el0 */         \
			"msr spsr_el1, xzr\n\t" /* Enable interrupt in EL0 -> Used for thread scheduler */         \
			"mov x0, %2\n\t"		/* Move destination address to x0 */                               \
			"eret\n\t"                                                                                 \
			:                                                                                          \
			: "r"(&curr_thread->context), "r"(curr_thread->context.lr), "r"(dest));                    \
	} while (0)

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

void run_user_task_wrapper(char *dest);
int exec(const char *name, char *const argv[]);
int fork();
void lock_interrupt();
void unlock_interrupt();

int sys_getpid(trapframe_t *tpf);
size_t sys_uart_read(trapframe_t *tpf, char buf[], size_t size);
size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size);
int sys_exec(trapframe_t *tpf, const char *name, char *const argv[]);
int sys_fork(trapframe_t *tpf);
int sys_exit(trapframe_t *tpf, int status);
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox);
int sys_kill(trapframe_t *tpf, int pid);
int sys_signal_register(trapframe_t *tpf, int SIGNAL, void (*handler)(void));
int sys_signal_kill(trapframe_t *tpf, int pid, int SIGNAL);
int sys_signal_return(trapframe_t *tpf);
void sys_lock_interrupt(trapframe_t *tpf);
void sys_unlock_interrupt(trapframe_t *tpf);

int kernel_fork();
int kernel_exec_user_program(const char *name, char *const argv[]);
void kernel_lock_interrupt();
void kernel_unlock_interrupt();

#endif /* _SYSCALL_H_ */