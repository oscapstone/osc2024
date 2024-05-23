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

// System call functions prefixed with sys_ are called by the exception handler
// Wrapper functions without the sys_ prefix are called by the user program

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
 * @brief Wrapper function to run the user task pointed to by curr_thread->code.
 */
void run_user_task_wrapper(char *dest)
{
	DEBUG("run_user_task_wrapper: 0x%x\r\n", dest);
	unlock_interrupt();
	((void (*)(void))dest)();
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
 * @brief Fork the current process.
 *
 * @param tpf Pointer to the trapframe structure.
 * @return The child's PID in the parent, 0 in the child.
 */
int sys_fork(trapframe_t *tpf)
{
	kernel_lock_interrupt();
	uint64_t sp, lr;
	asm volatile(
		"mov %0, sp\n"
		"mov %1, lr\n"
		: "=r"(sp), "=r"(lr));
	DEBUG("pid: %d, kernel stack: 0x%x -> 0x%x, kernel_stack_base: 0x%x, &(curr_thread->kernel_space_base): 0x%x\r\n", curr_thread->pid, sp, curr_thread->kernel_stack_base + KSTACK_SIZE, curr_thread->kernel_stack_base, &(curr_thread->kernel_stack_base));
	DEBUG("sp: 0x%x, lr: 0x%x\r\n", sp, lr);
	DEBUG("curr_thread->context.sp: 0x%x, curr_thread->context.fp: 0x%x\r\n", curr_thread->context.sp, curr_thread->context.fp);
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
	kernel_lock_interrupt();
	enum mbox_buffer_status_code status = mbox_call(ch, mbox);
	kernel_unlock_interrupt();
	return status;
}

int sys_kill(trapframe_t *tpf, int pid)
{
	thread_exit_by_pid(pid);
	return 0;
}

int sys_signal_register(trapframe_t *tpf, int SIGNAL, void (*haldler)(void))
{
	signal_register_handler(SIGNAL, haldler);
}

int sys_signal_kill(trapframe_t *tpf, int pid, int SIGNAL)
{
	signal_send(pid, SIGNAL);
}

int sys_signal_return(trapframe_t *tpf)
{
	signal_return();
}

/**
 * @brief Kernel function to fork the current process.
 *
 * @return The child's PID in the parent, 0 in the child.
 */
int kernel_fork()
{
	kernel_lock_interrupt();
	uint64_t sp, lr;
	asm volatile(
		"mov %0, sp\n"
		"mov %1, lr\n"
		: "=r"(sp), "=r"(lr));
	DEBUG("pid: %d, kernel stack: 0x%x -> 0x%x, kernel_stack_base: 0x%x, &(curr_thread->kernel_space_base): 0x%x\r\n", curr_thread->pid, sp, curr_thread->kernel_stack_base + KSTACK_SIZE, curr_thread->kernel_stack_base, &(curr_thread->kernel_stack_base));
	DEBUG("sp: 0x%x, lr: 0x%x\r\n", sp, lr);
	DEBUG("curr_thread->context.sp: 0x%x, curr_thread->context.fp: 0x%x\r\n", curr_thread->context.sp, curr_thread->context.fp);
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

	JUMP_TO_USER_SPACE(run_user_task_wrapper, curr_thread->code, curr_thread->user_stack_base + USTACK_SIZE, curr_thread->kernel_stack_base + KSTACK_SIZE);

	// curr_thread->context.lr = (uint64_t)run_user_task_wrapper;
	// asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
	// 	"msr elr_el1, %1\n\t"	// When el0 -> el1, store return address for el1 -> el0
	// 	"msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
	// 	"msr sp_el0, %2\n\t"	// el0 stack pointer for el1 process
	// 	"mov sp, %3\n\t"		// sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
	// 	"eret\n\t" ::"r"(&curr_thread->context),
	// 	"r"(curr_thread->context.lr), "r"(curr_thread->user_stack_base + USTACK_SIZE), "r"(curr_thread->kernel_stack_base + KSTACK_SIZE));
	return 0;
}
