#include "syscall.h"
#include "dev_framebuffer.h"
#include "initrd.h"
#include "irq.h"
#include "mbox.h"
#include "mm.h"
#include "proc.h"
#include "signal.h"
#include "string.h"
#include "traps.h"
#include "uart.h"
#include "utils.h"
#include "vfs.h"
#include "vfs_ramfs.h"
#include "vm.h"

extern unsigned int height;
extern unsigned int width;
extern unsigned int pitch;
extern unsigned int isrgb;

extern void child_ret_from_fork();

int sys_getpid()
{
    return get_current()->pid;
}

size_t sys_uart_read(char *buf, size_t size)
{
    int i = 0;
    while (i < size)
        buf[i++] = uart_getc();
    return i;
}

size_t sys_uart_write(const char *buf, size_t size)
{
    int i = 0;
    while (i < size)
        uart_putc(buf[i++]);
    return i;
}

int sys_exec(const char *name, char *const argv[])
{
    char path[PATH_MAX] = { 0 };
    realpath(name, path);
    struct vnode *target;
    vfs_lookup(path, &target);

    struct task_struct *task = get_current();
    task->start = ((struct ramfs_vnode *)target->internal)->data;
    task->code_size = ((struct ramfs_vnode *)target->internal)->datasize;

    // FIXME: Free the page table
    memset(task->pgd, 0, PAGE_SIZE);
    map_pages((unsigned long)task->pgd, 0x0, task->code_size,
              (unsigned long)VIRT_TO_PHYS(task->start), 0);
    map_pages((unsigned long)task->pgd, 0xFFFFFFFFB000, 0x4000,
              (unsigned long)VIRT_TO_PHYS(task->user_stack), 0);
    map_pages((unsigned long)task->pgd, 0x3C000000, 0x3000000, 0x3C000000, 0);

    task->sigpending = 0;
    memset(task->sighand, 0, sizeof(task->sighand));
    return 0;
}

int sys_fork(pt_regs *tf)
{
    disable_interrupt();
    struct task_struct *parent = get_current();
    struct task_struct *child = kthread_create(0);

    map_pages((unsigned long)child->pgd, 0x0, parent->code_size,
              VIRT_TO_PHYS((unsigned long)parent->start), 0);
    map_pages((unsigned long)child->pgd, 0xFFFFFFFFB000, 0x4000,
              (unsigned long)VIRT_TO_PHYS(child->user_stack), 0);
    map_pages((unsigned long)child->pgd, 0x3C000000, 0x3000000, 0x3C000000, 0);

    // Copy kernel stack and user stack
    memcpy(child->stack, parent->stack, STACK_SIZE);
    memcpy(child->user_stack, parent->user_stack, STACK_SIZE);

    // Copy signal handlers
    memcpy(child->sighand, parent->sighand, sizeof(parent->sighand));

    // Copy current working directory
    memcpy(child->cwd, parent->cwd, PATH_MAX);

    // Copy file descriptor table
    for (int i = 0; i < MAX_FD; i++) {
        if (parent->fdt[i]) {
            child->fdt[i] = kmalloc(sizeof(struct file));
            memcpy(child->fdt[i], parent->fdt[i], sizeof(struct file));
        }
    }

    unsigned long sp_off = (unsigned long)tf - (unsigned long)parent->stack;
    pt_regs *child_trap_frame = (pt_regs *)(child->stack + sp_off);

    child->context.lr = (unsigned long)child_ret_from_fork;
    child->context.sp = (unsigned long)child_trap_frame;
    child->context.fp = (unsigned long)child_trap_frame;

    child_trap_frame->sp_el0 = tf->sp_el0;
    child_trap_frame->x0 = 0;

    enable_interrupt();
    return child->pid;
}

void sys_exit()
{
    kthread_exit();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox)
{
    unsigned int *temp = kmalloc(mbox[0]);
    memcpy(temp, mbox, mbox[0]);
    int ret = mbox_call(ch, (unsigned int *)temp);
    memcpy(mbox, temp, mbox[0]);
    return ret;
}

void sys_kill(int pid)
{
    kthread_stop(pid);
}

void sys_signal(int signum, void (*handler)())
{
    signal(signum, handler);
}

void sys_sigkill(int pid, int sig)
{
    kill(pid, sig);
}

void sys_sigreturn(pt_regs *regs)
{
    // Restore the sigframe
    memcpy(regs, &get_current()->sigframe, sizeof(pt_regs));
    kfree(get_current()->sig_stack);
    get_current()->siglock = 0;
    return; // Jump to the previous context (user program) after eret
}

int sys_open(const char *pathname, int flags)
{
    char path[PATH_MAX] = { 0 };
    realpath(pathname, path);

    for (int i = 0; i < MAX_FD; i++)
        if (!get_current()->fdt[i])
            if (vfs_open(path, flags, &get_current()->fdt[i]) == 0)
                return i;
    return -1;
}

int sys_close(int fd)
{
    if (get_current()->fdt[fd]) {
        vfs_close(get_current()->fdt[fd]);
        get_current()->fdt[fd] = 0;
        return 0;
    }
    return -1;
}

long sys_write(int fd, const void *buf, unsigned long count)
{
    if (get_current()->fdt[fd])
        return vfs_write(get_current()->fdt[fd], buf, count);
    return -1;
}

long sys_read(int fd, void *buf, unsigned long count)
{
    if (get_current()->fdt[fd])
        return vfs_read(get_current()->fdt[fd], buf, count);
    return -1;
}

int sys_mkdir(const char *pathname, unsigned mode)
{
    char path[PATH_MAX] = { 0 };
    realpath(pathname, path);
    return vfs_mkdir(path);
}

int sys_mount(const char *src, const char *target, const char *filesystem,
              unsigned long flags, const void *data)
{
    char path[PATH_MAX] = { 0 };
    realpath(target, path);
    return vfs_mount(path, filesystem);
}

int sys_chdir(const char *path)
{
    char buf[PATH_MAX];
    realpath(path, buf);
    memset(get_current()->cwd, 0, PATH_MAX);
    strncpy(get_current()->cwd, buf, strlen(buf));
    return 0;
}

long sys_lseek64(int fd, long offset, int whence)
{
    if (whence == SEEK_SET) {
        get_current()->fdt[fd]->f_pos = offset;
        return offset;
    }
    return -1;
}

int sys_ioctl(int fd, unsigned long request, void *info)
{
    if (request == 0) {
        struct framebuffer_info *fb_info = info;
        fb_info->height = height;
        fb_info->width = width;
        fb_info->pitch = pitch;
        fb_info->isrgb = isrgb;
    }
    return 0;
}
