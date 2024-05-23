#include "stdint.h"
#include "sched.h"
#include "exception.h"
#include "stdio.h"
#include "cpio.h"
#include "string.h"
#include "mbox.h"
#include "bcm2837/rpi_mbox.h"

extern thread_t *curr_thread;
extern thread_t *threads[];

// Copy n bytes of memory from src to dest.
// MEMCPY(void *dest, const void *src, size_t n)
#define MEMCPY(dest, src, n)                          \
	do                                                \
	{                                                 \
		for (int i = 0; i < n; i++)                   \
		{                                             \
			*((char *)dest + i) = *((char *)src + i); \
		}                                             \
	} while (0)

// the function with sys_ prefix is the system call function that will be called by exception handler
// the function without sys_ prefix is the wrapper function that will be called by user program

int sys_getpid(trapframe_t *tpf)
{
	return curr_thread->pid;
}

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

size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size)
{
	for (int i = 0; i < size; i++)
	{
		uart_async_send(buf[i]);
	}
	return size;
}

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
	lock_interrupt();
	char *filepath = kmalloc(strlen(name) + 1);
	strcpy(filepath, name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	MEMCPY(curr_thread->code, filedata, filesize);
	// curr_thread->code = filedata;
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);
	tpf->elr_el1 = (uint64_t)curr_thread->code;
	tpf->sp_el0 = (uint64_t)curr_thread->user_stack_base + USTACK_SIZE;
	unlock_interrupt();
	return 0;
}

/**
 *  wrapper: run user task by curr_thread->code
 */
void run_user_task()
{
	DEBUG("run_user_task\r\n");
	asm volatile(
		"mov x8, #8\n\t"
		"svc 0\n\t"
		"mov x0, %0\n\t"
		"blr x0\n\t"
		:
		: "r"(curr_thread->code)
		: "x0", "x8");
}

/**
 * exec syscall in user space
 */
int exec(const char *name, char *const argv[])
{
	asm volatile(
		"mov x0, %0\n\t"
		"mov x1, %1\n\t"
		"mov x8, #4\n\t"
		"svc 0\n\t"
		:
		: "r"(name), "r"(argv)
		: "x0", "x1", "x8");
}

int sys_fork(trapframe_t *tpf)
{
	lock_interrupt();
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
	DEBUG("child pid: %d\r\n", pid);
	child->datasize = parent->datasize;
	child->status = THREAD_READY;

	child->code = kmalloc(parent->datasize);
	MEMCPY(child->code, parent->code, parent->datasize);
	DEBUG("fork child process, pid: %d, datasize: %d, code: 0x%x\r\n", child->pid, child->datasize, child->code);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(get_current_thread_context());
	// store_context(&(child->context));
	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);

	if (child->pid != curr_thread->pid) // process process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context = curr_thread->context;
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		unlock_interrupt();
		return child->pid;
	}
	else // child process
	{
		DEBUG("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		tpf = (trapframe_t *)((char *)tpf + (uint64_t)child->kernel_stack_base - (uint64_t)parent->kernel_stack_base); // move tpf
		tpf->sp_el0 += (uint64_t)child->user_stack_base - (uint64_t)parent->user_stack_base;
		return 0;
	}
}

int fork()
{
	int64_t pid;
	asm volatile(
		"mov x8, #4\n\t"
		"svc 0\n\t"
		"mov %0, x0\n\t"
		: "=r"(pid)
		:
		: "x8", "x0");

	return 0;
}

int sys_exit(trapframe_t *tpf, int status)
{
	thread_exit();
}

int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
	lock_interrupt();
	enum mbox_buffer_status_code status = mbox_call(ch, mbox);
	unlock_interrupt();
	return status;
}

int kernel_fork()
{
	lock_interrupt();
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
	child->code = parent->code;
	// child->code = kmalloc(parent->datasize);
	DEBUG("parent->code: 0x%x, child->code: 0x%x\r\n", parent->code, child->code);
	DEBUG("parent->datasize: %d, child->datasize: %d\r\n", parent->datasize, child->datasize);
	// MEMCPY(child->code, parent->code, parent->datasize);
	DEBUG("fork child process, pid: %d, datasize: %d, code: 0x%x\r\n", child->pid, child->datasize, child->code);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(get_current_thread_context());
	// store_context(&(child->context));
	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);

	if (child->pid != curr_thread->pid) // process process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context = curr_thread->context;
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		unlock_interrupt();
		return child->pid;
	}
	else // child process
	{
		DEBUG("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		return 0;
	}
}

int kernel_exec(const char *name, char *const argv[])
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

	DEBUG("kernel exec: %s\r\n", name);
	if (thread_code_can_free(curr_thread))
	{
		DEBUG("free code: 0x%x\r\n", curr_thread->code);
		kfree(curr_thread->code);
	}
	lock_interrupt();
	char *filepath = kmalloc(strlen(name) + 1);
	strcpy(filepath, name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	DEBUG("kernel exec: %s, code: 0x%x, filesize: %d\r\n", name, curr_thread->code, filesize);
	MEMCPY(curr_thread->code, filedata, filesize);

	// curr_thread->code = filedata;
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);

	// add_timer_by_sec(1, adapter_schedule_timer, NULL);
	// DEBUG("Start schedule timer after 1 sec\r\n");
	curr_thread->context.lr = (uint64_t)run_user_task;
	asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
		"msr elr_el1, %1\n\t"	// When el0 -> el1, store return address for el1 -> el0
		"msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
		"msr sp_el0, %2\n\t"	// el0 stack pointer for el1 process
		"mov sp, %3\n\t"		// sp is reference for the same el process. For example, el2 cannot use sp_el2, it has to use sp to find its own stack.
		"eret\n\t" ::"r"(&curr_thread->context),
		"r"(curr_thread->context.lr), "r"(curr_thread->user_stack_base + USTACK_SIZE), "r"(curr_thread->kernel_stack_base + KSTACK_SIZE));
	return 0;
}