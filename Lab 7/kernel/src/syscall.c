#include "syscall.h"
#include "uart.h"
#include "mailbox.h"
#include "initrd.h"
#include "sched.h"
#include "thread.h"
#include "signal.h"
#include "vfs.h"


int32_t
sys_getpid()
{
    return current_task()->pid;
}


uint32_t 
sys_uart_read(byteptr_t buf, uint32_t size)
{
    for (int32_t i = 0; i < size; i++) { buf[i] = uart_getc(); }
    return size;
}


uint32_t        
sys_uart_write(const byteptr_t buf, uint32_t size)
{
    for (int32_t i = 0; i < size; i++) { uart_put(buf[i]); }
    return size;
}


int32_t
sys_exec(const char *name, char *const argv[])
{
    initrd_exec_replace((const byteptr_t) name);
    return 0;
}


int32_t 
sys_fork()
{
    return thread_fork();
}


void
sys_exit(int status)
{
    thread_exit(status);
}


int32_t
sys_mbox_call(byte_t ch, uint32_t *mbox)
{
    return mailbox_call(ch, mbox);
}


void 
sys_kill_pid(int32_t pid)
{
    task_ptr tsk = scheduler_find(pid);
    if (tsk) scheduler_kill(tsk, 0);
}


void
sys_signal(int32_t sig_number, byteptr_t handler)
{
    uart_printf("syscall_signal (register) - signal number: %d, handler: 0x%x\n", sig_number, handler);
    task_ptr current = current_task();
    task_add_signal(current, sig_number, handler);
}


void
sys_sigkill(int32_t pid, int32_t sig_number)
{
    uart_printf("syscall_sigkill (signal) - number: %d, pid: %d\n", pid, sig_number);
    task_ptr tsk = scheduler_find(pid);

    if (tsk) task_to_signal_handler(tsk, sig_number);
    else uart_printf("[DEBUG] signal(%d) default handler\n", sig_number);
}


void
sys_sigreturn()
{
    uart_printf("syscall_sigreturn\n");
    task_ptr tsk = current_task();
    task_ret_from_signal_handler(tsk);
}



int32_t
syscall_open(const char *pathname, int flags)
{
    task_ptr current = current_task();
    for (int i = 0; i < FD_TABLE_SIZE; i++) {
        if (!current->fd_table[i] && !vfs_open(pathname, flags, &(current->fd_table[i]))) {
            return i;
        }
    }
    return -1;
}


int32_t
syscall_close(int fd)
{
    if (fd < 0 || fd >= FD_TABLE_SIZE) { return -1; }
    task_ptr current = current_task();
    if (current->fd_table[fd] && !vfs_close(current->fd_table[fd])) {
        current->fd_table[fd] = 0;
        return 0;
    }
    return -1;
}


int64_t
syscall_write(int fd, const void *buf, unsigned long count)
{
    if (fd < 0 || fd >= FD_TABLE_SIZE) { return -1; }
    task_ptr current = current_task();
    if (current->fd_table[fd]) { return vfs_write(current->fd_table[fd], buf, count); }
    return 0;
}


int64_t
syscall_read(int fd, void *buf, unsigned long count)
{
    if (fd < 0 || fd >= FD_TABLE_SIZE) { return -1; }
    task_ptr current = current_task();
    if (current->fd_table[fd]) { return vfs_read(current->fd_table[fd], buf, count); }
    return 0;
}


int32_t
syscall_mkdir(const char *pathname, unsigned mode)
{
    return vfs_mkdir(pathname);
}


int32_t
syscall_mount(const char *src, const char *target, const char *filesystem, uint64_t flags, const void *data)
{
    return vfs_mount(target, filesystem);
}


int32_t
syscall_chdir(const char *path)
{
    return vfs_chdir(path);
}
