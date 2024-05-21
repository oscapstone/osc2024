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

int syscall_getpid(trapframe_t *tpf)
{
	return curr_thread->pid;
}

size_t syscall_uart_read(trapframe_t *tpf, char buf[], size_t size)
{
	char c;
	for (int i = 0; i < size; i++)
	{
		c = uart_async_recv();
		buf[i] = c == '\r' ? '\n' : c;
	}
	return size;
}

size_t syscall_uart_write(trapframe_t *tpf, const char buf[], size_t size)
{
	for (int i = 0; i < size; i++)
	{
		uart_async_send(buf[i]);
	}
	return size;
}

int syscall_exec(trapframe_t *tpf, const char *name, char *const argv[])
{
	unsigned int filesize;
	char *filedata;
	int result = cpio_get_file(name, &filesize, &filedata);
	if (result == CPIO_TRAILER)
	{
		puts("exec: ");
		puts(name);
		puts(": No such file or directory\r\n");
		return -1;
	}
	else if (result == CPIO_ERROR)
	{
		puts("cpio parse error\r\n");
		return -1;
	}
	DEBUG("syscall_exec: %s\r\n", name);
	// if (!has_child(curr_thread))
	// {
	// 	kfree(curr_thread->code);
	// }
	lock_interrupt();
	char *filepath = kmalloc(strlen(name) + 1);
	strcpy(filepath, name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	MEMCPY(curr_thread->code, filedata, filesize);
	// curr_thread->code = filedata;
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);

	// curr_thread->code = kmalloc(filesize);
	// MEMCPY(curr_thread->code, filedata, filesize);
	tpf->elr_el1 = (uint64_t)curr_thread->code;
	tpf->sp_el0 = (uint64_t)curr_thread->user_stack_base + USTACK_SIZE;
	unlock_interrupt();
	return 0;
}

int syscall_fork(trapframe_t *tpf)
{
	lock_interrupt();
	uint64_t sp, lr;
	asm volatile(
		"mov %0, sp\n"
		"mov %1, lr\n"
		: "=r"(sp), "=r"(lr));
	DEBUG("pid: %d, kernel stack: 0x%x -> 0x%x, kernel_stack_base: 0x%x, &(curr_thread->kernel_space_base): 0x%x\r\n",curr_thread->pid, sp, curr_thread->kernel_stack_base + KSTACK_SIZE, curr_thread->kernel_stack_base, &(curr_thread->kernel_stack_base));
	DEBUG("sp: 0x%x, lr: 0x%x\r\n", sp, lr);
	DEBUG("curr_thread->context.sp: 0x%x, curr_thread->context.fp: 0x%x\r\n", curr_thread->context.sp, curr_thread->context.fp);
	thread_t *parent = curr_thread;
	char *new_name = kmalloc(strlen(parent->name) + 1);
	thread_t *child = thread_create(parent->code, new_name);
	int64_t pid = child->pid;
	DEBUG("child pid: %d\r\n", pid);
	child->datasize = parent->datasize;
	child->status = THREAD_READY;
	// child->code = parent->code;
	child->code = kmalloc(parent->datasize);
	DEBUG("fork child process, pid: %d, datasize: %d, code: 0x%x\r\n", child->pid, child->datasize, child->code);
	MEMCPY(child->code, parent->code, parent->datasize);
	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// the offset of current syscall should also be updated to new return point
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	child->context = parent->context;
	// store_context(&(child->context));
	store_context(get_current_thread_context());

	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);
	if (child->pid != curr_thread->pid) // process process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context = curr_thread->context;
		child->context.fp = (uint64_t)parent->context.fp - (uint64_t)parent->kernel_stack_base + (uint64_t)child->kernel_stack_base;
		child->context.sp = (uint64_t)parent->context.sp - (uint64_t)parent->kernel_stack_base + (uint64_t)child->kernel_stack_base;
		// child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		// child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		unlock_interrupt();
		return child->pid;
	}
	else
	{
		DEBUG("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		tpf = (trapframe_t *)((char *)tpf + (uint64_t)child->kernel_stack_base - (uint64_t)parent->kernel_stack_base); // move tpf
		tpf->sp_el0 += (uint64_t)child->user_stack_base - (uint64_t)parent->user_stack_base;
		return 0;
	}
}

int syscall_exit(trapframe_t *tpf, int status)
{
	thread_exit();
}

int syscall_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
	// 	lock_interrupt();
	// 	enum mbox_buffer_status_code status = mbox_call(ch, mbox);
	// 	unlock_interrupt();
	lock_interrupt();
	unsigned long r = (((unsigned long)((unsigned long)mbox) & ~0xF) | (ch & 0xF));
	do
	{
		asm volatile("nop");
	} while (*MBOX_STATUS & BCM_ARM_VC_MS_FULL);
	*MBOX_WRITE = r;
	while (1)
	{
		do
		{
			asm volatile("nop");
		} while (*MBOX_STATUS & BCM_ARM_VC_MS_EMPTY);
		if (r == *MBOX_READ)
		{
			tpf->x0 = (mbox[1] == MBOX_REQUEST_SUCCEED);
			unlock_interrupt();
			return mbox[1] == MBOX_REQUEST_SUCCEED;
		}
	}
	tpf->x0 = 0;
	unlock_interrupt();
	return 0;
}

int kernel_fork()
{
	lock_interrupt();
	char *new_name = kmalloc(strlen(curr_thread->name) + 1);
	thread_t *child = thread_create(curr_thread->code, new_name);
	int64_t pid = child->pid;
	child->datasize = curr_thread->datasize;
	child->status = THREAD_READY;
	MEMCPY(child->user_stack_base, curr_thread->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, curr_thread->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(&(child->context));
	if (child->pid != curr_thread->pid) // parent process
	{
		DEBUG("child->pid: %d, curr_thread->pid: %d\r\n", child->pid, curr_thread->pid);
		child->context.fp += child->kernel_stack_base - curr_thread->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - curr_thread->kernel_stack_base;
		unlock_interrupt();
		return child->pid;
	}
	else
	{
		return 0;
	}
}

void el0_jump_to_exec()
{
	unlock_interrupt();
	curr_thread->context.lr = curr_thread->code;
	void (*code_func)() = (void (*)())(curr_thread->code);
	code_func();
	// schedule();
}

int kernel_exec(const char *name, char *const argv[])
{
	char *c_filedata;
	char *filepath = kmalloc(strlen(name) + 1);
	strcpy(filepath, name);
	unsigned int c_filesize;

	int result = cpio_get_file(filepath, &c_filesize, &c_filedata);
	if (result == CPIO_TRAILER)
	{
		puts("exec: ");
		puts(filepath);
		puts(": No such file or directory\r\n");
		return -1;
	}
	else if (result == CPIO_ERROR)
	{
		puts("cpio parse error\r\n");
		return -1;
	}

	lock_interrupt();
	curr_thread->datasize = c_filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(c_filesize);
	curr_thread->context.lr = (uint64_t)el0_jump_to_exec;
	memcpy(curr_thread->code, c_filedata, c_filesize);
	DEBUG("c_filedata: 0x%x\r\n", c_filedata);
	DEBUG("child process, pid: %d\r\n", curr_thread->pid);
	DEBUG("curr_thread->code: 0x%x\r\n", curr_thread->code);

	asm("msr tpidr_el1, %0\n\t" // Hold the "kernel(el1)" thread structure information
		"msr elr_el1, %1\n\t"	// When el0 -> el1, store return address for el1 -> el0
		"msr spsr_el1, xzr\n\t" // Enable interrupt in EL0 -> Used for thread scheduler
		"msr sp_el0, %2\n\t"	// el0 stack pointer for el1 process
		"eret\n\t"				// Return to el0
		:
		: "r"(&curr_thread->context),
		  "r"(curr_thread->context.lr),
		  "r"(curr_thread->context.sp));

	return 0;
}