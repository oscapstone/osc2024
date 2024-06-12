#include "sys.h"
#include "schedule.h"
#include "fork.h"
#include "mini_uart.h"
#include "io.h"
#include "alloc.h"
#include "cpio.h"
#include "lib.h"
#include "type.h"
#include "string.h"
#include "peripherals/mailbox.h"
#include "dev_framebuffer.h"

extern struct task_struct *current;
extern void memzero_asm(unsigned long src, unsigned long n);
// #ifndef QEMU
// extern unsigned long CPIO_START_ADDR_FROM_DT;
// #endif

void foo(){};

int sys_getpid()
{
    return current->pid;
}

size_t sys_uartread(char buf[], size_t size)
{
    for(size_t i=0; i<size; i++){
        buf[i] = uart_recv();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size)
{
    for(size_t i=0; i<size; i++){
        uart_send(buf[i]);
    }
    return size;
}

int sys_exec(const char *name, char *const argv[]) // [TODO]
{
    struct file *file;
    int ret = vfs_open(name, O_RW, &file);
    if(ret == -1){
        printf("\r\n[ERROR] Cannot open file: "); printf(name);
        return -1;
    }

    int filesize = file->vnode->f_ops->getsize(file->vnode);
    void* user_program_addr = balloc(filesize+THREAD_SIZE);

    if(user_program_addr == NULL){
        printf("\r\n[ERROR] Cannot allocate memory for file: "); printf(name);
        return -1;
    }

    vfs_read(file, user_program_addr, filesize);
    vfs_close(file);

    preempt_disable(); // leads to get the correct current task

    struct pt_regs *cur_regs = task_pt_regs(current);
    cur_regs->pc = (unsigned long)user_program_addr;
    cur_regs->sp = current->stack + THREAD_SIZE;

    preempt_enable();
    return 0;
}

int sys_fork() // [TODO]
{
    unsigned long stack = (unsigned long)balloc(THREAD_SIZE);
    if((void*)stack == NULL) return -1;
    memzero_asm(stack, THREAD_SIZE);
    return copy_process(0, 0, 0, stack);
}

void sys_exit(int status) // [TODO]
{
    exit_process();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) // [TODO]
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] mbox_call: "); printf_int(ch);
#endif
    unsigned int mesg = (((unsigned int)(unsigned long)mbox) & ~0xf) | (ch & 0xf);
    while(*MAILBOX_STATUS & MAILBOX_FULL){   // // Check if Mailbox 0 status register’s full flag is set. if MAILBOX_STATUS == 0x80000001, then error parsing request buffer 
        asm volatile("nop");
    }

    *MAILBOX_WRITE = mesg;

    while(1){
        while(*MAILBOX_STATUS & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        if(mesg == *MAILBOX_READ){
            return mbox[1] == REQUEST_SUCCEED;
        }
    }
    return 0;
}

void sys_kill(int pid) // [TODO]
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] kill: "); printf_int(pid);
#endif
    if(task[pid] == NULL){
        printf("\r\nProcess not found: "); printf_int(pid);
        return;
    }
    task[pid]->state = TASK_ZOMBIE;
}

int sys_open(const char *pathname, int flags)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] open: "); printf(pathname); printf(", flags: "); printf_int(flags);
#endif
    for(int i=0; i<MAX_OPEN_FILE; i++){
        if(current->fd_table.fds[i] == NULL){
            int ret = vfs_open(pathname, flags, &current->fd_table.fds[i]);
            if(ret == 0){
                printf("\r\n[SYSCALL] open: File descriptor: "); printf_int(i + OFFSET_FD); printf(" , File: "); printf(pathname);
                return i + OFFSET_FD;
            }
            break;
        }
    }
    return -1;
}

int sys_close(int fd)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] close: "); printf_int(fd);
#endif
    if(fd < OFFSET_FD || fd >= MAX_OPEN_FILE + OFFSET_FD) return -1;
    int ret = vfs_close(current->fd_table.fds[fd - OFFSET_FD]);
    current->fd_table.fds[fd - OFFSET_FD] = NULL;
    return ret;
}

long sys_write(int fd, const void *buf, unsigned long count)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] write: fd: "); printf_int(fd); printf(", count: "); printf_int(count);
#endif
    if(fd < OFFSET_FD || fd >= MAX_OPEN_FILE + OFFSET_FD) return -1;
    return vfs_write(current->fd_table.fds[fd - OFFSET_FD], buf, count);
}
long sys_read(int fd, void *buf, unsigned long count)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] read: fd: "); printf_int(fd); printf(", count: "); printf_int(count);
#endif
    if(fd < OFFSET_FD || fd >= MAX_OPEN_FILE + OFFSET_FD) return -1;
    return vfs_read(current->fd_table.fds[fd - OFFSET_FD], buf, count);
}

// you can ignore mode, since there is no access control
int sys_mkdir(const char *pathname, unsigned mode)
{
    return vfs_mkdir(pathname);
}

// you can ignore arguments other than target (where to mount) and filesystem (fs name)
int sys_mount(const char *src, const char *target, const char *filesystem, unsigned long flags, const void *data)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] mount:"); printf(src); printf(" to "); printf(target); printf(" with filesystem: "); printf(filesystem);
#endif
    return vfs_mount(target, filesystem);
}

int sys_chdir(const char *path)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] chdir: "); printf(path);
#endif
    return vfs_chdir(path);
}


long sys_lseek64(int fd, long offset, int whence)
{
    long ret;
    if(whence == SEEK_SET) // used for dev_framebuffer
    {
        current->fd_table.fds[fd - OFFSET_FD]->f_pos = offset;
        ret = offset;
    }
    else // other is not supported
    {
        ret = -1;
    }

    return ret;
}

// ioctl 0 will be use to get info
// there will be default value in info
// if it works with default value, you can ignore this syscall
long sys_ioctl(int fd, unsigned long request, unsigned long arg)
{
#ifdef SYSCALL_DEBUG
    printf("\r\n[SYSCALL] ioctl: fd: "); printf_int(fd); printf(" , request: "); printf_hex(request);
#endif
    return 0;
}


void * const sys_call_table[] = {
        sys_getpid,
        sys_uartread,
        sys_uartwrite,
        sys_exec,
        sys_fork,
        sys_exit,
        sys_mbox_call,
        sys_kill,
        // lab5 advanced
        foo,
        foo,
        foo,
        // lab7
        sys_open,
        sys_close,
        sys_write,
        sys_read,
        sys_mkdir,
        sys_mount,
        sys_chdir,
        sys_lseek64,
        sys_ioctl
    };
