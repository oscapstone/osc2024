#ifndef _SYSCALL_H_
#define _SYSCALL_H_

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
		{                                                                                              \
			DEBUG("Set kernel stack pointer: 0x%x\n", kernel_sp);                                      \
			asm volatile("mov sp, %0\n\t" ::"r"(kernel_sp)); /* set kernel stack pointer */            \
		}                                                                                              \
		if (user_sp != NULL)                                                                           \
		{                                                                                              \
			DEBUG("Set user stack pointer: 0x%x\n", user_sp);                                          \
			asm volatile("msr sp_el0, %0\n\t" ::"r"(user_sp)); /* el0 stack pointer for el1 process */ \
		}                                                                                              \
		DEBUG("PID: %d\n", curr_thread->pid);                                                          \
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
	SYSCALL_MMAP,
	SYSCALL_OPEN,
	SYSCALL_CLOSE,
	SYSCALL_WRITE,
	SYSCALL_READ,
	SYSCALL_MKDIR,
	SYSCALL_MOUNT,
	SYSCALL_CHDIR,
	SYSCALL_LSEEK64,
	SYSCALL_IOCTL,
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
void *sys_mmap(trapframe_t *tpf, void *addr, size_t len, int prot, int flags, int fd, int file_offset);
int sys_open(trapframe_t *tpf, const char *pathname, int flags);
int sys_close(trapframe_t *tpf, int fd);
long sys_write(trapframe_t *tpf, int fd, const void *buf, unsigned long count);
long sys_read(trapframe_t *tpf, int fd, void *buf, unsigned long count);
int sys_mkdir(trapframe_t *tpf, const char *pathname, unsigned mode);
int sys_mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int sys_chdir(trapframe_t *tpf, const char *path);
long sys_lseek64(trapframe_t *tpf, int fd, long offset, int whence);
int sys_ioctl(trapframe_t *tpf, int fb, unsigned long request, void *info);
int sys_signal_return(trapframe_t *tpf);
void sys_lock_interrupt(trapframe_t *tpf);
void sys_unlock_interrupt(trapframe_t *tpf);

int kernel_fork();
int kernel_exec_user_program(const char *name, char *const argv[]);
int kernel_open(const char *pathname, int flags);
int kernel_close(int fd);
long kernel_write(int fd, const void *buf, unsigned long count);
long kernel_read(int fd, void *buf, unsigned long count);
int kernel_mkdir(const char *pathname, unsigned mode);
int kernel_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data);
int kernel_chdir(const char *path);
long kernel_lseek64(int fd, long offset, int whence);
int kernel_ioctl(int fb, unsigned long request, void *info);
void kernel_lock_interrupt();
void kernel_unlock_interrupt();

#endif /* _SYSCALL_H_ */