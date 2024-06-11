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
    struct task_struct* p;
    for(int i=0; i<NR_TASKS; i++){
        if(task[i] == NULL) continue; // (task[i] == NULL) means no more tasks
        p = task[i];
        if(p->pid == pid){
            preempt_disable();
            printf("\r\nKilling process: "); printf_int(pid);
            p->state = TASK_ZOMBIE;
            preempt_enable();
            return;
        }
    }
    printf("\r\nProcess not found: "); printf_int(pid);
    return;
}

int sys_open(const char *pathname, int flags)
{
    printf("\r\n[SYSCALL] open: "); printf(pathname); printf(", flags: "); printf_int(flags);
    for(int i=0; i<MAX_OPEN_FILE; i++){
        if(current->fd_table.fds[i] == NULL){
            int ret = vfs_open(pathname, flags, &current->fd_table.fds[i]);
            if(ret == 0){
                printf("\r\n[SYSCALL OPEN] File descriptor: "); printf_int(i + OFFSET_FD); printf(" , File: "); printf(pathname);
                return i + OFFSET_FD;
            }
            break;
        }
    }
    return -1;
}

int sys_close(int fd)
{
    printf("\r\n[SYSCALL] close: "); printf_int(fd);
    if(fd < OFFSET_FD || fd >= MAX_OPEN_FILE + OFFSET_FD) return -1;
    int ret = vfs_close(current->fd_table.fds[fd - OFFSET_FD]);
    current->fd_table.fds[fd - OFFSET_FD] = NULL;
    return ret;
}

long sys_write(int fd, const void *buf, unsigned long count)
{
    printf("\r\n[SYSCALL] write: fd: "); printf_int(fd); printf(", count: "); printf_int(count);
    if(fd < OFFSET_FD || fd >= MAX_OPEN_FILE + OFFSET_FD) return -1;
    return vfs_write(current->fd_table.fds[fd - OFFSET_FD], buf, count);
}
long sys_read(int fd, void *buf, unsigned long count)
{
    printf("\r\n[SYSCALL] read: fd: "); printf_int(fd); printf(", count: "); printf_int(count);
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
    printf("\r\n[SYSCALL] mount:"); printf(src); printf(" to "); printf(target); printf(" with filesystem: "); printf(filesystem);
    return vfs_mount(target, filesystem);
}
int sys_chdir(const char *path)
{
    return vfs_chdir(path);
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
        sys_chdir
    };