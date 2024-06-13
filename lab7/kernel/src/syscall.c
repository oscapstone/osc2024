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
#include "memory.h"

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
	// DEBUG("run_user_task_wrapper: 0x%x\r\n", dest);
	// unlock_interrupt();
	CALL_SYSCALL(SYSCALL_UNLOCK_INTERRUPT);

	((void (*)(void))dest)();
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
	DEBUG("getpid: %d\r\n", curr_thread->pid);
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
		// c = uart_recv();
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
		// uart_send(buf[i]);
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
	kernel_lock_interrupt();
	char *filepath = kmalloc(strlen(name) + 1);
	strcpy(filepath, name);
	curr_thread->datasize = filesize;
	curr_thread->name = filepath;
	curr_thread->code = kmalloc(filesize);
	MEMCPY(curr_thread->code, filedata, filesize);
	curr_thread->user_stack_bottom = kmalloc(USTACK_SIZE);

	asm("dsb ish\n\t"); // ensure write has completed
	mmu_clean_page_tables(curr_thread->context.pgd, LEVEL_PGD);
	memset(PHYS_TO_KERNEL_VIRT(curr_thread->context.pgd), 0, 0x1000);
	mmu_free_all_vma(curr_thread);
	set_thread_default_mmu(curr_thread);

	uint64_t ttbr0_el1_value;
	asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0_el1_value));
	DEBUG("PGD: 0x%x, ttbr0_el1: 0x%x\r\n", curr_thread->context.pgd, ttbr0_el1_value);
	DEBUG("curr_thread->pid: %d\r\n", curr_thread->pid);
	DEBUG_BLOCK({
		dump_vma(curr_thread);
	});
	tpf->elr_el1 = (uint64_t)USER_CODE_BASE;
	tpf->sp_el0 = (uint64_t)USER_STACK_BASE - SP_OFFSET_FROM_TOP;

	flush_tlb();
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

	child->code = parent->code;

	MEMCPY(child->kernel_stack_bottom, parent->kernel_stack_bottom, KSTACK_SIZE);
	list_head_t *pos;
	DEBUG("Copy vma_list from %d to %d\r\n", parent->pid, child->pid);
	list_for_each(pos, (list_head_t *)(curr_thread->vma_list))
	{
		vm_area_struct_t *vma = (vm_area_struct_t *)pos;
		mmu_add_vma(child, vma->name, vma->start, vma->end - vma->start, vma->vm_page_prot, vma->vm_flags, vma->vm_file);
	}

	DEBUG_BLOCK({
		DEBUG("-------------- Child VMA --------------\r\n");
		dump_vma(child);
		DEBUG("-------------- Parent VMA --------------\r\n");
		dump_vma(parent);
	});
	DEBUG("parent->context.pgd: 0x%x, child->context.pgd: 0x%x\r\n", parent->context.pgd, child->context.pgd);
	mmu_copy_page_table_and_set_read_only(child->context.pgd, parent->context.pgd, LEVEL_PGD);
	flush_tlb();

	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(&(child->context));
	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);
	DEBUG("parent->context.pgd: 0x%x, child->context.pgd: 0x%x\r\n", parent->context.pgd, child->context.pgd);

	if (child->pid != curr_thread->pid) // Parent process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		child->context.fp += child->kernel_stack_bottom - parent->kernel_stack_bottom;
		child->context.sp += child->kernel_stack_bottom - parent->kernel_stack_bottom;
		kernel_unlock_interrupt();
		return child->pid;
	}
	else // Child process
	{
		DEBUG("set_tpf: pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		tpf = (trapframe_t *)((char *)tpf + (uint64_t)child->kernel_stack_bottom - (uint64_t)parent->kernel_stack_bottom); // move tpf
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
int sys_mbox_call(trapframe_t *tpf, unsigned char ch, unsigned int *mbox_user)
{
	// kernel_lock_interrupt();
	// enum mbox_buffer_status_code status = mbox_call(ch, mbox);
	// kernel_unlock_interrupt();

	kernel_lock_interrupt();

	unsigned int size_of_mbox = mbox_user[0];
	memcpy((char *)pt, mbox_user, size_of_mbox);
	mbox_call(MBOX_TAGS_ARM_TO_VC, (unsigned int)((unsigned long)&pt));
	memcpy(mbox_user, (char *)pt, size_of_mbox);

	kernel_unlock_interrupt();

	return 8;
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

// only need to implement the anonymous page mapping in this Lab.
void *sys_mmap(trapframe_t *tpf, void *addr, size_t len, int prot, int flags, int fd, int file_offset)
{
	// Ignore flags as we have demand pages
	DEBUG("mmap addr: 0x%x, len: 0x%x\r\n", addr, len);

	// Req #3 Page size round up
	len = ALIGN_UP(len, PAGE_FRAME_SIZE);
	addr = ALIGN_UP((uint64_t)addr, PAGE_FRAME_SIZE);

	// Req #2 check if overlap
	list_head_t *pos;
	vm_area_struct_t *vma;
	size_t new_addr = (size_t)addr;
	int found = 0;
	while (!found)
	{
		found = 1; // 假設找到一個合適的位置
		list_for_each(pos, (list_head_t *)curr_thread->vma_list)
		{
			vma = (vm_area_struct_t *)pos;
			// Detect existing vma overlapped
			if (!(vma->start >= (new_addr + len) || vma->end <= new_addr))
			{
				// 如果有重疊，則更新 new_addr 並重新檢查
				new_addr = vma->end;
				found = 0; // 還沒找到合適的位置
				break;
			}
		}
	}

	// create new valid region, map and set the page attributes (prot)
	DEBUG("mmap: new_addr: 0x%x, len: 0x%x, prot: 0x%x, flags: 0x%x\r\n", new_addr, len, prot, flags);
	mmu_add_vma(curr_thread, "anon", new_addr, len, prot, flags, NULL);
	INFO_BLOCK({
		dump_vma(curr_thread);
	});
	return (void *)new_addr;
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

int sys_mkdir(trapframe_t *tpf, const char *pathname, unsigned mode){
	return kernel_mkdir(pathname, mode);
}

int sys_mount(trapframe_t *tpf, const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data){
	return kernel_mount(src, target, filesystem, flags, data);
}

int sys_chdir(trapframe_t *tpf, const char *path){
	return kernel_chdir(path);
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

	MEMCPY(child->user_stack_bottom, parent->user_stack_bottom, USTACK_SIZE);
	MEMCPY(child->kernel_stack_bottom, parent->kernel_stack_bottom, KSTACK_SIZE);
	// Because make a function call, so lr is the next instruction address
	// When context switch, child process will start from the next instruction
	store_context(&(child->context));
	DEBUG("child: 0x%x, parent: 0x%x\r\n", child, parent);
	DEBUG("curr_thread->pid: %d, curr_thread->context.pgd: 0x%x, parent->context.pgd: 0x%x, child->context.pgd: 0x%x\r\n", curr_thread->pid, curr_thread->context.pgd, parent->context.pgd, child->context.pgd);

	if (child->pid != curr_thread->pid) // Parent process
	{
		DEBUG("pid: %d, child: 0x%x, child->pid: %d, curr_thread: 0x%x, curr_thread->pid: %d\r\n", pid, child, child->pid, curr_thread, curr_thread->pid);
		// child->context = curr_thread->context;
		child->context.fp += child->kernel_stack_bottom - parent->kernel_stack_bottom;
		child->context.sp += child->kernel_stack_bottom - parent->kernel_stack_bottom;
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
	DEBUG("kernel exec: %s, code: 0x%x, filesize: %d, pid: %d\r\n", program_name, curr_thread->code, filesize, curr_thread->pid);
	MEMCPY(curr_thread->code, filedata, filesize);
	curr_thread->user_stack_bottom = kmalloc(USTACK_SIZE);

	set_thread_default_mmu(curr_thread);

	uint64_t ttbr0_el1_value;
	asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr0_el1_value));
	DEBUG("PGD: 0x%x, ttbr0_el1: 0x%x\r\n", curr_thread->context.pgd, ttbr0_el1_value);
	DEBUG("curr_thread->pid: %d\r\n", curr_thread->pid);
	// 讀取 TTBR0_EL1 的值
	DEBUG("VA of run_user_task_wrapper: 0x%x, USER_RUN_USER_TASK_WRAPPER_VA: 0x%x\r\n", run_user_task_wrapper, USER_RUN_USER_TASK_WRAPPER_VA);
	DEBUG("VA of signal_handler_wrapper: 0x%x, USER_SIGNAL_WRAPPER_VA: 0x%x\r\n", signal_handler_wrapper, USER_SIGNAL_WRAPPER_VA);
	DEBUG("VA of user sp: 0x%x", USER_STACK_BASE);
	// JUMP_TO_USER_SPACE(USER_RUN_USER_TASK_WRAPPER_VA + (uint64_t)run_user_task_wrapper % PAGE_FRAME_SIZE, USER_CODE_BASE, USER_STACK_BASE, curr_thread->kernel_stack_bottom + KSTACK_SIZE - SP_OFFSET_FROM_TOP);
	JUMP_TO_USER_SPACE(USER_RUN_USER_TASK_WRAPPER_VA, USER_CODE_BASE, USER_STACK_BASE, curr_thread->kernel_stack_bottom + KSTACK_SIZE - SP_OFFSET_FROM_TOP);
	return 0;
}

int kernel_open(const char *pathname, int flags)
{
	file_t *file;
	if (vfs_open(curr_thread->pwd, pathname, flags, &file) != 0)
	{
		return -1;
	}
	int fd = thread_insert_fd_to_table(file);
	if (fd == -1)
	{
		ERROR("sys_open: fd is full\r\n");
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

int kernel_mkdir(const char *pathname, unsigned mode){
	return vfs_mkdir(curr_thread->pwd, pathname);
}

int kernel_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data){
	return vfs_mount(curr_thread->pwd, target, filesystem);
}

int kernel_chdir(const char *path){
	vnode_t *target;
	if(vfs_lookup(curr_thread->pwd, path, &target) != 0){
		return -1;
	}
	curr_thread->pwd = target;
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
