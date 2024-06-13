/*
 * Functions prefixed with sys_ are called by the exception handler.
 * Functions prefixed with kernel_ can be called in kernel-space.
 * Functions without any prefix are system call functions called in user-space.
 */

#include "stdint.h"
#include "sched.h"
#include "exception.h"
#include "stdio.h"
#include "cpio.h"
#include "string.h"
#include "mbox.h"
#include "bcm2837/rpi_mbox.h"
#include "list.h"
#include "syscall.h"
#include "signal.h"

// External references to the current thread and the thread array
extern thread_t *curr_thread;

// Macro to copy n bytes of memory from src to dest
#define MEMCPY(dest, src, n)                          \
	do                                                \
	{                                                 \
		for (int i = 0; i < n; i++)                   \
		{                                             \
			*((char *)dest + i) = *((char *)src + i); \
		}                                             \
	} while (0)

/**
 * @brief Wrapper function to run the user task pointed to by curr_thread->code.
 */
void run_user_task_wrapper(char *dest)
{
	((void (*)(void))dest)();
	CALL_SYSCALL(SYSCALL_EXIT);
}

/**
 * @brief Wrapper function for fork syscall in user space.
 *
 * @return The child's PID in the parent, 0 in the child.
 */
int fork()
{
	int64_t pid;
	CALL_SYSCALL(SYSCALL_FORK);
	asm volatile(
		"mov %0, x0\n\t"
		: "=r"(pid)
		:
		: "x0");
	return pid;
}

/**
 * @brief User-space syscall to execute a program.
 *
 * @param name Name of the program to execute.
 * @param argv Arguments to pass to the program.
 * @return 0 on success, -1 on failure.
 */
int exec(const char *name, char *const argv[])
{
	asm volatile(
		"mov x0, %0\n\t"
		"mov x1, %1\n\t"
		:
		: "r"(name), "r"(argv)
		: "x0", "x1");
	CALL_SYSCALL(SYSCALL_EXEC);
}

/**
 * @brief User-space syscall to lock the interrupt.
 */
void lock_interrupt()
{
	CALL_SYSCALL(SYSCALL_LOCK_INTERRUPT);
}

/**
 * @brief User-space syscall to unlock the interrupt.
 */
void unlock_interrupt()
{
	CALL_SYSCALL(SYSCALL_UNLOCK_INTERRUPT);
}

/**
 * @brief Get the process ID of the current thread.
 *
 * @param tpf Pointer to the trapframe structure.
 * @return The process ID of the current thread.
 */
int sys_getpid(trapframe_t *tpf)
{
	return curr_thread->pid;
}

/**
 * @brief Read from UART into a buffer.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param buf Buffer to store the read characters.
 * @param size Number of characters to read.
 * @return The number of characters read.
 */
size_t sys_uart_read(trapframe_t *tpf, char buf[], size_t size)
{
	char c;
	for (int i = 0; i < size; i++)
	{
		c = uart_async_recv();
		buf[i] = c == '\r' ? '\n' : c;
	}
	return size;
}

/**
 * @brief Write a buffer to UART.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param buf Buffer containing characters to write.
 * @param size Number of characters to write.
 * @return The number of characters written.
 */
size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size)
{
	for (int i = 0; i < size; i++)
	{
		uart_async_send(buf[i]);
	}
	return size;
}

/**
 * @brief Load and execute a user program.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param name Name of the program to execute.
 * @param argv Arguments to pass to the program.
 * @return 0 on success, -1 on failure.
 */
int sys_exec(trapframe_t *tpf, const char *name, char *const argv[])
{
	unsigned int filesize;
	char *filedata;
	int result = cpio_get_file(name, &filesize, &filedata);
	if (result == CPIO_TRAILER)
	{
		WARNING("exec: %s: No such file or directory\r\n", name);
		return -1;
	}
	else if (result == CPIO_ERROR)
	{
		ERROR("cpio parse error\r\n");
		return -1;
	}

	DEBUG("sys_exec: %s\r\n", name);
	if (thread_code_can_free(curr_thread))
	{
		kfree(curr_thread->code);
	}
	kernel_lock_interrupt();
	char *filepath = kmalloc(strlen(name) + 1);
	strcpy(filepath, name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	MEMCPY(curr_thread->code, filedata, filesize);
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);
	tpf->elr_el1 = (uint64_t)curr_thread->code;
	tpf->sp_el0 = (uint64_t)curr_thread->user_stack_base + USTACK_SIZE;
	kernel_unlock_interrupt();
	return 0;
}

/**
 * @brief Fork the current process.
 *
 * @param tpf Pointer to the trapframe structure.
 * @return The child's PID in the parent, 0 in the child.
 */
int sys_fork(trapframe_t *tpf)
{
	kernel_lock_interrupt();
	thread_t *parent = curr_thread;
	char *new_name = kmalloc(strlen(parent->name) + 1);
	strcpy(new_name, parent->name);
	thread_t *child = thread_create(parent->code, new_name);
	int64_t pid = child->pid;
	DEBUG("child pid: %d\r\n", pid);
	child->datasize = parent->datasize;
	child->status = THREAD_READY;
	SIGNAL_COPY(child, parent);

	child->code = kmalloc(parent->datasize);
	MEMCPY(child->code, parent->code, parent->datasize);
	DEBUG("fork child process, pid: %d, datasize: %d, code: 0x%x\r\n", child->pid, child->datasize, child->code);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(&(child->context));
	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);

	if (child->pid != curr_thread->pid) // Parent process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		kernel_unlock_interrupt();
		return child->pid;
	}
	else // Child process
	{
		DEBUG("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		tpf = (trapframe_t *)((char *)tpf + (uint64_t)child->kernel_stack_base - (uint64_t)parent->kernel_stack_base); // move tpf
		tpf->sp_el0 += (uint64_t)child->user_stack_base - (uint64_t)parent->user_stack_base;
		return 0;
	}
}

/**
 * @brief Exit the current process.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param status Exit status.
 * @return This function does not return.
 */
int sys_exit(trapframe_t *tpf, int status)
{
	thread_exit();
}

/**
 * @brief Make a mailbox call.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param ch Mailbox channel.
 * @param mbox Pointer to the mailbox buffer.
 * @return Status code of the mailbox call.
 */
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
	// kernel_lock_interrupt();
	enum mbox_buffer_status_code status = mbox_call(ch, mbox);
	// kernel_unlock_interrupt();
	return status;
}

/**
 * @brief Terminate a thread by process ID.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param pid Process ID of the thread to be terminated.
 * @return Always returns 0.
 */
int sys_kill(trapframe_t *tpf, int pid)
{
	thread_exit_by_pid(pid);
	return 0;
}

/**
 * @brief Register a signal handler.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param SIGNAL The signal number for which the handler is being registered.
 * @param handler Pointer to the handler function to be registered.
 */
int sys_signal_register(trapframe_t *tpf, int SIGNAL, void (*handler)(void))
{
	signal_register_handler(SIGNAL, handler);
}

/**
 * @brief Send a signal to a process.
 *
 * @param tpf Pointer to the trapframe structure.
 * @param pid Process ID of the target process.
 * @param SIGNAL The signal number to be sent.
 */
int sys_signal_kill(trapframe_t *tpf, int pid, int SIGNAL)
{
	signal_send(pid, SIGNAL);
}

/**
 * @brief Before returning to the execution of the signal handler.
 *
 * @param tpf Pointer to the trapframe structure.
 */
int sys_signal_return(trapframe_t *tpf)
{
	signal_return();
}

/**
 * @brief Lock the interrupt.
 *
 * @param tpf Pointer to the trapframe structure.
 */
void sys_lock_interrupt(trapframe_t *tpf)
{
	__lock_interrupt();
}

/**
 * @brief Unlock the interrupt.
 *
 * @param tpf Pointer to the trapframe structure.
 */
void sys_unlock_interrupt(trapframe_t *tpf)
{
	DEBUG("__unlock_interrupt\r\n");
	__unlock_interrupt();
}

/**
 * @brief Kernel function to fork the current process.
 *
 * @return The child's PID in the parent, 0 in the child.
 */
int kernel_fork()
{
	kernel_lock_interrupt();
	thread_t *parent = curr_thread;
	char *new_name = kmalloc(strlen(parent->name) + 1);
	thread_t *child = thread_create(parent->code, new_name);
	int64_t pid = child->pid;
	DEBUG("child pid: %d, datasize: %d\r\n", pid, parent->datasize);
	child->datasize = parent->datasize;
	child->status = THREAD_READY;
	SIGNAL_COPY(child, parent);

	child->code = parent->code;
	DEBUG("parent->code: 0x%x, child->code: 0x%x\r\n", parent->code, child->code);
	DEBUG("parent->datasize: %d, child->datasize: %d\r\n", parent->datasize, child->datasize);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(get_current_thread_context());
	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);

	if (child->pid != curr_thread->pid) // Parent process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context = curr_thread->context;
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		kernel_unlock_interrupt();
		return child->pid;
	}
	else // Child process
	{
		DEBUG("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		return 0;
	}
}

/**
 * @brief Kernel function to execute a user-space program.
 *
 * @param program_name The name of the program to execute.
 * @param argv Arguments to pass to the program.
 * @return 0 on success, -1 on failure.
 */
int kernel_exec_user_program(const char *program_name, char *const argv[])
{
	unsigned int filesize;
	char *filedata;
	int result = cpio_get_file(program_name, &filesize, &filedata);
	if (result == CPIO_TRAILER)
	{
		WARNING("exec: %s: No such file or directory\r\n", program_name);
		return -1;
	}
	else if (result == CPIO_ERROR)
	{
		ERROR("cpio parse error\r\n");
		return -1;
	}

	DEBUG("kernel exec: %s\r\n", program_name);
	if (thread_code_can_free(curr_thread))
	{
		DEBUG("free code: 0x%x\r\n", curr_thread->code);
		kfree(curr_thread->code);
	}
	kernel_lock_interrupt();
	char *filepath = kmalloc(strlen(program_name) + 1);
	strcpy(filepath, program_name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	DEBUG("kernel exec: %s, code: 0x%x, filesize: %d\r\n", program_name, curr_thread->code, filesize);
	MEMCPY(curr_thread->code, filedata, filesize);
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);
	kernel_unlock_interrupt();
	JUMP_TO_USER_SPACE(run_user_task_wrapper, curr_thread->code, curr_thread->user_stack_base + USTACK_SIZE, curr_thread->kernel_stack_base + KSTACK_SIZE);
	return 0;
}

/**
 * @brief Kernel function to lock the interrupt.
 */
void kernel_lock_interrupt()
{
	__lock_interrupt();
}

/**
 * @brief Kernel function to unlock the interrupt.
 */
void kernel_unlock_interrupt()
{
	__unlock_interrupt();
}
