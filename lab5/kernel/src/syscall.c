#include "peripherals/rpi_mbox.h"
#include "mbox.h"
#include "syscall.h"
#include "sched.h"
#include "mini_uart.h"
#include "utils.h"
#include "exception.h"
#include "memory.h"
#include "cpio.h"

extern thread_t     *curr_thread;
extern void         *CPIO_START;


// Macro to copy n bytes of memory from src to dest
#define MEMCPY(dest, src, n)                          \
	do {                                              \
		for (int i = 0; i < n; i++) {                 \
			*((char *)dest + i) = *((char *)src + i); \
		}                                             \
	} while (0)


void run_user_task_wrapper(char *dest) {
	// uart_puts("run_user_task_wrapper: 0x%x\r\n", dest);
    // print_lock();
	((void (*)(void))dest)();
}

/* Get the current PID */
int sys_getpid(trapframe_t *tpf) {
	return curr_thread->pid;
}

/* Read from UART and put into buffer */
size_t sys_uart_read(trapframe_t *tpf, char buf[], size_t size) {
	char c;
	for (int i = 0; i < size; i++)
	{
		c = uart_async_getc();
		buf[i] = c == '\r' ? '\n' : c;
	}
	return size;
}

/* Write from buffer to UART */
size_t sys_uart_write(trapframe_t *tpf, const char buf[], size_t size) {
	for (int i = 0; i < size; i++)
	{
		uart_async_send(buf[i]);
	}
	return size;
}

/* Load and execute a user program. */
int sys_exec(trapframe_t *tpf, const char *filepath, char *const argv[]) {
    char* c_filepath;
    char* c_filedata;
    unsigned int c_filesize;
    struct cpio_newc_header *header_ptr = CPIO_START;

    while(header_ptr != 0) {
        int err = cpio_parse_header(header_ptr, &c_filepath, &c_filesize, &c_filedata, &header_ptr);
        if (err) {
            uart_puts("CPIO parse error\r\n");
            return -1;
        }
        if (header_ptr != 0 && strcmp(filepath, c_filepath) == 0) {
            break; 
        }
        if (header_ptr == 0) { 
            uart_puts("cat: %s: No such file or directory\n", filepath);
            return -1;
        }
    }

	uart_puts("sys_exec: %s\r\n", filepath);
	if (thread_code_can_free(curr_thread)) {
		kfree(curr_thread->code);
	}
	lock();
	char *name = kmalloc(strlen(filepath) + 1);
	strcpy(name, filepath);
	curr_thread->datasize = c_filesize;
	curr_thread->name = name;
	curr_thread->code = kmalloc(c_filesize);
	MEMCPY(curr_thread->code, c_filedata, c_filesize);
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);
	tpf->elr_el1 = (uint64_t)curr_thread->code;
	tpf->sp_el0 = (uint64_t)curr_thread->user_stack_base + USTACK_SIZE;
	unlock();
	return 0;
}

int sys_fork(trapframe_t *tpf) {
	lock();
	thread_t *parent = curr_thread;
	char *new_name = kmalloc(strlen(parent->name) + 1);
	strcpy(new_name, parent->name);
	thread_t *child = thread_create(parent->code, new_name);
	int64_t pid = child->pid;
	// uart_puts("child pid: %d\r\n", pid);
	child->datasize = parent->datasize;
	child->status = THREAD_READY;
	// SIGNAL_COPY(child, parent);

	child->code = kmalloc(parent->datasize);
	MEMCPY(child->code, parent->code, parent->datasize);
	// uart_puts("fork child process, pid: %d, datasize: %d, code: 0x%x\r\n", child->pid, child->datasize, child->code);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(&(child->context));
	// uart_puts("child: 0x%x, parent: 0x%x\r\n", child, parent);

    /* Parent process */ 
	if (child->pid != curr_thread->pid) {
		// uart_puts("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		unlock();
		return child->pid;
	}
    /* Child process */ 
	else {
		uart_puts("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		tpf = (trapframe_t *)((char *)tpf + (uint64_t)child->kernel_stack_base - (uint64_t)parent->kernel_stack_base); // move tpf
		tpf->sp_el0 += (uint64_t)child->user_stack_base - (uint64_t)parent->user_stack_base;
		return 0;
	}
}

int kernel_fork() {
	lock();
	thread_t *parent = curr_thread;
	char *new_name = kmalloc(strlen(parent->name) + 1);
	thread_t *child = thread_create(parent->code, new_name);
	// int64_t pid = child->pid;
	child->datasize = parent->datasize;
	child->status = THREAD_READY;
    // uart_puts("child pid: %d, datasize: %d\r\n", pid, parent->datasize);

	// SIGNAL_COPY(child, parent);

	child->code = parent->code;
	// uart_puts("parent->code: 0x%x, child->code: 0x%x\r\n", parent->code, child->code);
	// uart_puts("parent->datasize: %d, child->datasize: %d\r\n", parent->datasize, child->datasize);

	MEMCPY(child->user_stack_base, parent->user_stack_base, USTACK_SIZE);
	MEMCPY(child->kernel_stack_base, parent->kernel_stack_base, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(get_current());
	// uart_puts("child: 0x%x, parent: 0x%x\r\n", child, parent);

	if (child->pid != curr_thread->pid) // Parent process
	{
		// uart_puts("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context = curr_thread->context;
		child->context.fp += child->kernel_stack_base - parent->kernel_stack_base;
		child->context.sp += child->kernel_stack_base - parent->kernel_stack_base;
		unlock();
		return child->pid;
	}
	else // Child process
	{
		// uart_puts("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		return 0;
	}
}

/* Exit current process */
int sys_exit(trapframe_t *tpf, int status) {
	thread_exit();
    return 0;
}

/* Make a mailbox call */
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int mbox) {
	int ret = mbox_call(ch, mbox);
	return ret;
}

/* Terminate a thread by PID */
int sys_kill(trapframe_t *tpf, int pid) {
	thread_exit_by_pid(pid);
	return 0;
}

/* Kernel function to execute user program */
int kernel_exec_user_program(const char *program_name, char *const argv[]) { 
	unsigned int filesize;
	char        *filedata;
	int result = cpio_get_file(program_name, &filesize, &filedata);
	if (result == CPIO_TRAILER)
	{
		// uart_puts("exec: %s: No such file or directory\r\n", program_name);
		return -1;
	}
	else if (result == CPIO_ERROR)
	{
		// uart_puts("cpio parse error\r\n");
		return -1;
	}

	// uart_puts("kernel exec: %s\r\n", program_name);
	if (thread_code_can_free(curr_thread))
	{
		// uart_puts("free code: 0x%x\r\n", curr_thread->code);
		kfree(curr_thread->code);
	}
	char *filepath = kmalloc(strlen(program_name) + 1);
	strcpy(filepath, program_name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	// uart_puts("kernel exec: %s, code: 0x%x, filesize: %d\r\n", program_name, curr_thread->code, filesize);
	MEMCPY(curr_thread->code, filedata, filesize);
	curr_thread->user_stack_base = kmalloc(USTACK_SIZE);

	JUMP_TO_USER_SPACE(run_user_task_wrapper, curr_thread->code, curr_thread->user_stack_base + USTACK_SIZE, curr_thread->kernel_stack_base + KSTACK_SIZE);
	return 0;
}
