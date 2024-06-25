#include "peripherals/rpi_mbox.h"
#include "mbox.h"
#include "syscall.h"
#include "sched.h"
#include "mini_uart.h"
#include "utils.h"
#include "exception.h"
#include "memory.h"
#include "cpio.h"
#include "signal.h"
#include "vfs.h"
#include "dev_framebuffer.h"

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
	file_t *file;
	vfs_open(curr_thread->pwd, filepath, O_RDONLY, &file);

	uart_puts("sys_exec: %s\r\n", filepath);
	lock();
	curr_thread->file = file;
	curr_thread->datasize = file->f_ops->getsize(file->vnode);
	curr_thread->name = filepath;
	curr_thread->code = NULL;
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
	signal_copy(child, parent);
	FD_TABLE_COPY(child, parent);

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
		// uart_puts("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
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

	signal_copy(child, parent);

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
	// lock();
	uart_puts("KERNEL EXEC\r\n");

	file_t *file;
	uart_puts("%s, 0x%x\r\n", curr_thread->pwd, curr_thread->pwd);
	vfs_open(curr_thread->pwd, program_name, O_RDONLY, &file);
	
	char *filepath = kmalloc(strlen(program_name) + 1);
	strcpy(filepath, program_name);
	curr_thread->datasize = file->f_ops->getsize(file->vnode);;
	curr_thread->name = filepath;
	// curr_thread->code = kmalloc(file->f_ops->getsize(file->vnode));
	uart_puts("0x%x\r\n", file->f_ops->getsize(file->vnode));
	// vfs_read(file, curr_thread->code, curr_thread->datasize);
	// curr_thread->user_stack_base = kmalloc(USTACK_SIZE);
	// uart_puts("kernel exec: %s, code: 0x%x, filesize: %d\r\n", program_name, curr_thread->code, curr_thread->datasize);

	// JUMP_TO_USER_SPACE(run_user_task_wrapper, curr_thread->code, curr_thread->user_stack_base + USTACK_SIZE, curr_thread->kernel_stack_base + KSTACK_SIZE);
	return 0;
}


/* Signal Part */
int sys_signal_register(trapframe_t *tpf, int SIGNAL, void (*handler)(void)) {
	signal_register_handler(SIGNAL, handler);
}

int sys_signal_kill(trapframe_t *tpf, int pid, int SIGNAL) {
	signal_send(pid, SIGNAL);
}

int sys_signal_return(trapframe_t *tpf) {
	signal_return();
}

void sys_lock_interrupt(trapframe_t *tpf) {
	lock();
}

void sys_unlock_interrupt(trapframe_t *tpf) {
	unlock();
}

int sys_open(trapframe_t *tpf, const char *pathname, int flags)
{
	return kernel_open(pathname, flags);
}

int sys_close(trapframe_t *tpf, int fd)
{
	return kernel_close(fd);
}

long sys_write(trapframe_t *tpf, int fd, const void *buf, unsigned long count)
{
	return kernel_write(fd, buf, count);
}

long sys_read(trapframe_t *tpf, int fd, void *buf, unsigned long count)
{
	return kernel_read(fd, buf, count);
}

int sys_mkdir(trapframe_t *tpf, const char *pathname, unsigned mode)
{
	return kernel_mkdir(pathname, mode);
}

int sys_mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
	return kernel_mount(src, target, filesystem, flags, data);
}

int sys_chdir(trapframe_t *tpf, const char *path)
{
	return kernel_chdir(path);
}

long sys_lseek64(trapframe_t *tpf, int fd, long offset, int whence)
{
	return kernel_lseek64(fd, offset, whence);
}

extern unsigned int height;
extern unsigned int isrgb;
extern unsigned int pitch;
extern unsigned int width;
int sys_ioctl(trapframe_t *tpf, int fb, unsigned long request, void *info)
{
	return kernel_ioctl(fb, request, info);
}

int kernel_open(const char *pathname, int flags)
{
	file_t *file;
	if (vfs_open(curr_thread->pwd, pathname, flags, &file) != 0)
	{
		return -1;
	}
	int fd = thread_insert_file_to_fdtable(curr_thread, file);
	if (fd == -1)
	{
		uart_puts("sys_open: fd is full\r\n");
		vfs_close(file);
		return -1;
	}
	return fd;
}

int kernel_close(int fd)
{
	file_t *file;
	if (thread_get_file_struct_by_fd(fd, &file) == -1)
		return -1;
	vfs_close(file);
	curr_thread->file_descriptors_table[fd] = NULL;
	return 0;
}

long kernel_write(int fd, const void *buf, unsigned long count)
{
	file_t *file;
	if (thread_get_file_struct_by_fd(fd, &file) == -1)
		return -1;
	return vfs_write(file, buf, count);
}

long kernel_read(int fd, void *buf, unsigned long count)
{
	file_t *file;
	if (thread_get_file_struct_by_fd(fd, &file) == -1)
		return -1;
	return vfs_read(file, buf, count);
}

int kernel_mkdir(const char *pathname, unsigned mode)
{
	return vfs_mkdir(curr_thread->pwd, pathname);
}

int kernel_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
	return vfs_mount(curr_thread->pwd, target, filesystem);
}

int kernel_chdir(const char *path)
{
	vnode_t *target;
	if (vfs_lookup(curr_thread->pwd, path, &target) != 0)
	{
		return -1;
	}
	curr_thread->pwd = target;
	return 0;
}

long kernel_lseek64(int fd, long offset, int whence)
{
	if (whence == SEEK_SET) // used for dev_framebuffer
	{
		curr_thread->file_descriptors_table[fd]->f_pos = offset;
		return offset;
	}
	else // other is not supported
	{
		return -1;
	}
}

extern unsigned int height;
extern unsigned int isrgb;
extern unsigned int pitch;
extern unsigned int width;
int kernel_ioctl(int fb, unsigned long request, void *info)
{
	if (request == 0) // used for get info (SPEC)
	{
		struct framebuffer_info *fb_info = info;
		fb_info->height = height;
		fb_info->isrgb = isrgb;
		fb_info->pitch = pitch;
		fb_info->width = width;
	}
	return 0;
}
