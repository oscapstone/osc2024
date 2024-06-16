#include "memory.h"
#include "timer.h"
#include "utility.h"
#include "stdint.h"
#include "syscall.h"
#include "exception.h"
#include "mini_uart.h"
#include "shell.h"
#include "sched.h"
#include "mbox.h"
#include "cpio.h"

extern thread_t *curr_thread;

#define MEMCPY(dest, src, n)                          \
    do                                                \
    {                                                 \
        for (int i = 0; i < n; i++)                   \
        {                                             \
            *((char *)dest + i) = *((char *)src + i); \
        }                                             \
    } while (0)

int sys_getpid(trapframe_t *tpf)
{
    return curr_thread->pid;
}
size_t sys_uart_read(trapframe_t *tpf, char buf[], size_t size)
{
    char c;
    for (int i = 0; i < size; i++)
    {
        c = uart_async_getc();
        buf[i] = c == '\r' ? '\n' : c;
    }
    return size;
}
size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size)
{
    for (int i = 0; i < size; i++)
    {
        uart_async_putc(buf[i]);
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
        // WARNING("exec: %s: No such file or directory\r\n", name);
        return -1;
    }
    else if (result == CPIO_ERROR)
    {
        // ERROR("cpio parse error\r\n");
        return -1;
    }

    // //("sys_exec: %s\r\n", name);
    // if (thread_code_can_free(curr_thread))
    // {
    //     kfree(curr_thread->code);
    // }
    lock();
    char *filepath = kmalloc(strlen(name) + 1);
    strcpy(filepath, name);
    curr_thread->datasize = filesize;
    curr_thread->name = filepath;
    curr_thread->code = kmalloc(filesize);
    MEMCPY(curr_thread->code, filedata, filesize);
    curr_thread->user_stack_base = kmalloc(USTACK_SIZE);
    tpf->elr_el1 = (uint64_t)curr_thread->code;
    tpf->sp_el0 = (uint64_t)curr_thread->user_stack_base + USTACK_SIZE;
    unlock();
    return 0;
}
int sys_fork(trapframe_t *tpf)
{
    lock();
    thread_t *parent = curr_thread;
    char *new_name = kmalloc(strlen(parent->name) + 1);
    strcpy(new_name, parent->name);
    thread_t *child = thread_create(parent->code, new_name);
    int64_t pid = child->pid;
    //("child pid: %d\r\n", pid);
    child->datasize = parent->datasize;
    child->status = THREAD_IS_READY;
    // SIGNAL_COPY(child, parent);

    child->code = kmalloc(parent->datasize);
    MEMCPY(child->code, parent->code, parent->datasize);
    //("fork child process, pid: %d, datasize: %d, code: 0x%x\r\n", child->pid, child->datasize, child->code);

    MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
    MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
    // Because make a function call, so lr is the next instruction address
    // When context switch, child process will start from the next instruction
    store_context(&(child->context));
    //("child: 0x%x, parent: 0x%x\r\n", child, parent);

    if (child->pid != curr_thread->pid) // Parent process
    {
        //("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
        child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
        child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
        unlock();
        return child->pid;
    }
    else // Child process
    {
        //("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
        tpf = (trapframe_t *)((char *)tpf + (uint64_t)child->kernel_stack_base - (uint64_t)parent->kernel_stack_base); // move tpf
        tpf->sp_el0 += (uint64_t)child->user_stack_base - (uint64_t)parent->user_stack_base;
        return 0;
    }
}

int sys_exit(trapframe_t *tpf, int status)
{
    thread_exit();
}
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox)
{
}
int sys_kill(trapframe_t *tpf, int pid)
{
    // thread_exit_by_pid(pid);
    return 0;
}

int kernel_fork()
{
	lock();
	thread_t *parent = curr_thread;
	char *new_name = kmalloc(strlen(parent->name) + 1);
	thread_t *child = thread_create(parent->code, new_name);
	int64_t pid = child->pid;
	//("child pid: %d, datasize: %d\r\n", pid, parent->datasize);
	child->datasize = parent->datasize;
	child->status = THREAD_IS_READY;
	// SIGNAL_COPY(child, parent);

	child->code = parent->code;
	//("parent->code: 0x%x, child->code: 0x%x\r\n", parent->code, child->code);
	//("parent->datasize: %d, child->datasize: %d\r\n", parent->datasize, child->datasize);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(get_current_thread_context());
	//("child: 0x%x, parent: 0x%x\r\n", child, parent);

	if (child->pid != curr_thread->pid) // Parent process
	{
		//("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context = curr_thread->context;
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		unlock();
		return child->pid;
	}
	else // Child process
	{
		//("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		return 0;
	}
}